/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "redstone_manager.h"
#include "helpers/direction_fixer.h"
#include "helpers/java/java_hash_set.h"
#include "logger/logger.h"
#include "world.h"
#include <deque>
#include <unordered_set>
#include <vector>

static std::deque<RedstoneUpdateInfo> torchUpdates;

// BlockRedstoneWire.blocksNeedingUpdate. Java has one of these on the single wire Block instance,
// so it is shared by every dimension and keeps its grown capacity for the life of the server.
static Java::ChunkPositionHashSet blocksNeedingUpdate;

void RedstoneManager::SetJavaHashMapVersion(Java::HashMapVersion _version) {
	blocksNeedingUpdate.SetVersion(_version);
}

void RedstoneManager::PruneTorchUpdates(WorldManager& _world) {
	while (!torchUpdates.empty() && _world.elapsedTicks - torchUpdates.front().updateTime > 100)
		torchUpdates.pop_front();
}

bool RedstoneManager::CheckTorchBurnout(WorldManager& _world, Int3 _pos, bool _logUpdate) {
	if (_logUpdate)
		torchUpdates.push_back({ _pos.x, _pos.y, _pos.z, _world.elapsedTicks });

	int count = 0;
	for (auto& entry : torchUpdates) {
		if (entry.x == _pos.x && entry.y == _pos.y && entry.z == _pos.z) {
			if (++count >= 8)
				return true;
		}
	}
	return false;
}

static bool IsPoweredByAttachedLeverOrButton(WorldManager& _world, Int3 _pos) {
	static constexpr Direction::Value ALL_DIRS[6] = {
		Direction::Value::North, Direction::Value::South, Direction::Value::East,
		Direction::Value::West,  Direction::Value::Up,    Direction::Value::Down,
	};
	for (auto dir : ALL_DIRS) {
		Int3 neighborPos = _pos.WithOffset(dir);
		BlockType neighborBlock = _world.GetBlockId(neighborPos);
		if (neighborBlock != BLOCK_LEVER && neighborBlock != BLOCK_BUTTON_STONE)
			continue;

		uint8_t neighborMeta = _world.GetMetadata(neighborPos);
		if (!(neighborMeta & 0b1000))
			continue; // Not switched on

		// A lever/button only powers the block it is actually mounted against
		if (GetDirectionFromMeta(neighborBlock, neighborMeta) == dir)
			return true;
	}
	return false;
}

bool RedstoneManager::CanBridgeVertical(WorldManager& _world, Int3 _pos, int _dx, int _dz, int _dyOffset) {
	bool sideIsSolid = _world.IsBlockNormalCube({ _pos.x + _dx, _pos.y, _pos.z + _dz });

	if (_dyOffset > 0) {
		bool openAboveUs = !_world.IsBlockNormalCube({ _pos.x, _pos.y + 1, _pos.z });
		return sideIsSolid && openAboveUs;
	}

	return !sideIsSolid;
}

ComponentProfile RedstoneManager::GetRedstoneDustConnectivity(WorldManager& _world, Int3 _pos) {
	// Check if this redstone dust is being redirected, and if so, where its being redirected to
	ComponentProfile thisProfile;

	// Horizontal scan
	for (int dy = -1 + _pos.y; dy <= 1 + _pos.y; dy++) {
		int d[4] = { -1, 1, 0, 0 };
		for (int i = 0; i < 4; i++) {
			auto rdx = d[i];
			auto rdz = d[3 - i];
			auto dx = rdx + _pos.x;
			auto dz = rdz + _pos.z;

			BlockType neighborBlock = _world.GetBlockId({ dx, dy, dz });
			bool canConnect = (dy == _pos.y) || (CanBridgeVertical(_world, _pos, rdx, rdz, dy - _pos.y) &&
			                                     RedstoneManager::CanProvidePower(neighborBlock));

			bool continuesHere = RedstoneManager::CanProvidePower(neighborBlock);

			// Repeaters depend on facing direction
			if (dy == _pos.y &&
			    (neighborBlock == BLOCK_REDSTONE_REPEATER_ON || neighborBlock == BLOCK_REDSTONE_REPEATER_OFF)) {
				continuesHere = false;
				auto profile = RedstoneManager::GetComponentProfile(BLOCK_REDSTONE_REPEATER_ON,
				                                                    _world.GetMetadata({ dx, dy, dz }));
				if (rdx == -1) {
					if (profile.powerNX)
						continuesHere = true;
				}
				if (rdx == 1) {
					if (profile.powerX)
						continuesHere = true;
				}
				if (rdz == -1) {
					if (profile.powerNZ)
						continuesHere = true;
				}
				if (rdz == 1) {
					if (profile.powerZ)
						continuesHere = true;
				}
			}

			if (continuesHere && canConnect) {
				if (rdx == -1) {
					thisProfile.powerNX = true;
				}
				if (rdx == 1) {
					thisProfile.powerX = true;
				}
				if (rdz == -1) {
					thisProfile.powerNZ = true;
				}
				if (rdz == 1) {
					thisProfile.powerZ = true;
				}
			}
		}
	}

	return thisProfile;
}

bool RedstoneManager::DustPowersToward(WorldManager& _world, Int3 _dustPos, int _dx, int _dz) {
	if (_world.GetBlockId(_dustPos) != BLOCK_REDSTONE || _world.GetMetadata(_dustPos) == 0)
		return false;

	auto c = GetRedstoneDustConnectivity(_world, _dustPos);

	// A lone dot powers every side
	if (!c.powerNX && !c.powerX && !c.powerNZ && !c.powerZ)
		return true;

	// Otherwise the line has to run straight into the block
	if (_dx != 0) {
		bool farSide = _dx < 0 ? c.powerNX : c.powerX;
		return farSide && !c.powerNZ && !c.powerZ;
	}
	bool farSide = _dz < 0 ? c.powerNZ : c.powerZ;
	return farSide && !c.powerNX && !c.powerX;
}

PowerProfile RedstoneManager::GetBlockPowerProfile(WorldManager& _world, Int3 _pos) {
	// Only normal cubes conduct power
	if (!_world.IsBlockNormalCube(_pos))
		return {};

	bool softPowered = false;

	// Check if a switched-on lever or button is mounted against us
	if (IsPoweredByAttachedLeverOrButton(_world, _pos)) {
		return { true, true };
	}

	// Is a powered pressure plate above us?
	auto above = _world.GetBlockId(_pos.WithOffset(Direction::Value::Up));
	auto aboveMeta = _world.GetMetadata(_pos.WithOffset(Direction::Value::Up));
	if (above == BLOCK_PRESSURE_PLATE_STONE || above == BLOCK_PRESSURE_PLATE_WOOD) {
		if (aboveMeta == 1)
			return { true, true };
	}

	// Check below us
	if (_world.GetBlockId({ _pos.x, _pos.y - 1, _pos.z }) == BLOCK_REDSTONE_TORCH_ON) {
		return { true, true };
	}

	// Check above us
	if (above == BLOCK_REDSTONE && aboveMeta > 0) {
		softPowered = true;
	}

	// Check sides
	int d[4] = { -1, 1, 0, 0 };
	for (int i = 0; i < 4; i++) {
		auto rdx = d[i];
		auto rdz = d[3 - i];
		auto dx = rdx + _pos.x;
		auto dz = rdz + _pos.z;

		Int3 thisPos = { dx, _pos.y, dz };

		// Redstone dust only soft powers us if it points into us
		if (DustPowersToward(_world, thisPos, rdx, rdz))
			softPowered = true;

		// This is a repeater, see if it is facing us and powered
		if (_world.GetBlockId(thisPos) == BLOCK_REDSTONE_REPEATER_ON) {
			auto grc = RedstoneManager::GetComponentProfile(BLOCK_REDSTONE_REPEATER_ON, _world.GetMetadata(thisPos));
			if (rdx == 1) {
				if (grc.powerNX)
					return { true, true };
			} else if (rdx == -1) {
				if (grc.powerX)
					return { true, true };
			} else if (rdz == 1) {
				if (grc.powerNZ)
					return { true, true };
			} else if (rdz == -1) {
				if (grc.powerZ)
					return { true, true };
			}
		}
	}

	// We were marked as soft powered but couldn't find any harder power
	if (softPowered)
		return { true, false };
	return {}; // Block isn't powered
}

// Java's isBlockIndirectlyGettingPowered for a wire, with wiresProvidePower off
static bool DustHasExternalPower(WorldManager& _world, Int3 _pos) {
	//auto thisBlock = _world.GetBlockId(_pos);
	//auto thisMeta = _world.GetMetadata(_pos);

	// Is the block under us being hard powered?
	if (RedstoneManager::GetBlockPowerProfile(_world, _pos.WithOffset(Direction::Value::Down)).hardPowered)
		return true;

	// Is the block above us being hard powered?
	if (RedstoneManager::GetBlockPowerProfile(_world, _pos.WithOffset(Direction::Value::Up)).hardPowered)
		return true;

	// Can the block above us power us?
	if (RedstoneManager::GetComponentProfile(_world.GetBlockId(_pos.WithOffset(Direction::Value::Up)),
	                                         _world.GetMetadata(_pos.WithOffset(Direction::Value::Up)))
	        .powerBelow)
		return true;

	// Check the blocks to the side of us
	int d[4] = { -1, 1, 0, 0 };
	for (int i = 0; i < 4; i++) {
		auto rdx = d[i];
		auto rdz = d[3 - i];
		auto dx = rdx + _pos.x;
		auto dz = rdz + _pos.z;

		for (int dy = _pos.y - 1; dy <= _pos.y + 1; dy++) {
			Int3 dPos = { dx, dy, dz };
			auto checkId = _world.GetBlockId(dPos);
			auto checkMeta = _world.GetMetadata(dPos);
			// We only care if we are being hard powered from the same Y level
			if (dy == _pos.y && RedstoneManager::GetBlockPowerProfile(_world, dPos).hardPowered)
				return true;

			// A torch sitting directly beside us
			if (dy == _pos.y && checkId == BLOCK_REDSTONE_TORCH_ON) {
				auto torchMeta = checkMeta;
				auto torchProfile = RedstoneManager::GetComponentProfile(BLOCK_REDSTONE_TORCH_ON, torchMeta);

				// Direction FROM the torch TOWARD us
				bool torchPowersUs = false;
				if (rdx == 1)
					torchPowersUs = torchProfile.powerNX;
				else if (rdx == -1)
					torchPowersUs = torchProfile.powerX;
				else if (rdz == 1)
					torchPowersUs = torchProfile.powerNZ;
				else if (rdz == -1)
					torchPowersUs = torchProfile.powerZ;

				if (torchPowersUs)
					return true;
			}

			// A repeater sitting directly beside us
			if (dy == _pos.y && checkId == BLOCK_REDSTONE_REPEATER_ON) {
				auto repeaterMeta = checkMeta;
				auto repeaterProfile = RedstoneManager::GetComponentProfile(BLOCK_REDSTONE_REPEATER_ON, repeaterMeta);

				// Direction FROM the repeater TOWARD us
				bool repeaterPowersUs = false;
				if (rdx == 1)
					repeaterPowersUs = repeaterProfile.powerNX;
				else if (rdx == -1)
					repeaterPowersUs = repeaterProfile.powerX;
				else if (rdz == 1)
					repeaterPowersUs = repeaterProfile.powerNZ;
				else if (rdz == -1)
					repeaterPowersUs = repeaterProfile.powerZ;

				if (repeaterPowersUs)
					return true;
			}

			// A lever or button or pressure plate sitting directly beside us
			if (dy == _pos.y && (checkId == BLOCK_LEVER || checkId == BLOCK_BUTTON_STONE ||
			                     checkId == BLOCK_PRESSURE_PLATE_STONE || checkId == BLOCK_PRESSURE_PLATE_WOOD)) {
				auto neighborBlock = _world.GetBlockId(dPos);
				auto neighborMeta = _world.GetMetadata(dPos);
				auto neighborProfile = RedstoneManager::GetComponentProfile(neighborBlock, neighborMeta);

				// Direction FROM the lever/button TOWARD us
				bool poweredTowardUs = false;
				if (rdx == 1)
					poweredTowardUs = neighborProfile.powerNX;
				else if (rdx == -1)
					poweredTowardUs = neighborProfile.powerX;
				else if (rdz == 1)
					poweredTowardUs = neighborProfile.powerNZ;
				else if (rdz == -1)
					poweredTowardUs = neighborProfile.powerZ;

				if (poweredTowardUs)
					return true;
			}

		}
	}

	return false;
}

// The strongest wire this one connects to (same level, or a valid vertical bridge)
static int GetDustNeighborLevel(WorldManager& _world, Int3 _pos) {
	int best = 0;
	int d[4] = { -1, 1, 0, 0 };
	for (int i = 0; i < 4; i++) {
		auto rdx = d[i];
		auto rdz = d[3 - i];
		for (int dy = _pos.y - 1; dy <= _pos.y + 1; dy++) {
			Int3 dPos = { rdx + _pos.x, dy, rdz + _pos.z };
			bool canConnect = (dy == _pos.y) || RedstoneManager::CanBridgeVertical(_world, _pos, rdx, rdz, dy - _pos.y);
			if (canConnect && _world.GetBlockId(dPos) == BLOCK_REDSTONE) {
				auto neighborLevel = _world.GetMetadata(dPos);
				if (neighborLevel > best)
					best = neighborLevel;
			}
		}
	}
	return best;
}

static int GetDustPowerLevel(WorldManager& _world, Int3 _pos) {
	if (DustHasExternalPower(_world, _pos))
		return 15;
	int best = GetDustNeighborLevel(_world, _pos);
	return best > 0 ? best - 1 : 0;
}

bool RedstoneManager::IsPositionPowered(WorldManager& _world, Int3 _pos) {
	// Is the block under us being powered?
	if (RedstoneManager::GetBlockPowerProfile(_world, _pos.WithOffset(Direction::Value::Down)).powered)
		return true;

	// Is the block above us being powered?
	if (RedstoneManager::GetBlockPowerProfile(_world, _pos.WithOffset(Direction::Value::Up)).powered)
		return true;

	// Can the block above us power us?
	if (RedstoneManager::GetComponentProfile(_world.GetBlockId(_pos.WithOffset(Direction::Value::Up)),
	                                         _world.GetMetadata(_pos.WithOffset(Direction::Value::Up)))
	        .powerBelow)
		return true;

	// Check the blocks to the side of us
	int d[4] = { -1, 1, 0, 0 };
	for (int i = 0; i < 4; i++) {
		auto rdx = d[i];
		auto rdz = d[3 - i];
		Int3 dPos = { rdx + _pos.x, _pos.y, rdz + _pos.z };
		auto neighborBlock = _world.GetBlockId(dPos);

		if (RedstoneManager::GetBlockPowerProfile(_world, dPos).powered)
			return true;

		if (neighborBlock == BLOCK_REDSTONE_TORCH_ON || neighborBlock == BLOCK_REDSTONE_REPEATER_ON ||
		    neighborBlock == BLOCK_LEVER || neighborBlock == BLOCK_BUTTON_STONE ||
		    neighborBlock == BLOCK_PRESSURE_PLATE_STONE || neighborBlock == BLOCK_PRESSURE_PLATE_WOOD) {
			auto neighborProfile = RedstoneManager::GetComponentProfile(neighborBlock, _world.GetMetadata(dPos));

			// Direction FROM the neighbor TOWARD us
			bool poweredTowardUs = false;
			if (rdx == 1)
				poweredTowardUs = neighborProfile.powerNX;
			else if (rdx == -1)
				poweredTowardUs = neighborProfile.powerX;
			else if (rdz == 1)
				poweredTowardUs = neighborProfile.powerNZ;
			else if (rdz == -1)
				poweredTowardUs = neighborProfile.powerZ;

			if (poweredTowardUs)
				return true;
		}

		// Dust beside us only counts if it points into us, not if it just runs past
		if (neighborBlock == BLOCK_REDSTONE && DustPowersToward(_world, dPos, rdx, rdz))
			return true;
	}

	return false;
}

static void GetNeighbors(WorldManager& _world, Int3 _pos, std::unordered_set<Int3>& _visited, std::vector<Int3>& _order,
                         bool _forceDisableProfileCheck = false) {
	auto thisBlock = _world.GetBlockId(_pos);
	bool doProfileCheck = false;

	// Only mark visited if we are redstone dust
	if (thisBlock == BLOCK_REDSTONE) {
		if (!_visited.insert(_pos).second)
			return;
		_order.push_back(_pos);
	}

	ComponentProfile thisProfile;
	if (thisBlock != BLOCK_REDSTONE && !_forceDisableProfileCheck) {
		// Make the profile getter use the powered repeater since the unpowered repeater will return false for every direction
		thisProfile = RedstoneManager::GetComponentProfile(
		    thisBlock == BLOCK_REDSTONE_REPEATER_OFF ? BLOCK_REDSTONE_REPEATER_ON : thisBlock, _world.GetMetadata(_pos));
		doProfileCheck = true;
	}

	std::vector<Int3> neighbors;
	// Horizontal scan
	for (int dy = -1 + _pos.y; dy <= 1 + _pos.y; dy++) {
		int d[4] = { -1, 1, 0, 0 };
		for (int i = 0; i < 4; i++) {
			auto rdx = d[i];
			auto rdz = d[3 - i];
			auto dx = rdx + _pos.x;
			auto dz = rdz + _pos.z;

			if (doProfileCheck) {
				if (rdx == -1) {
					if (!thisProfile.powerNX)
						continue;
				}
				if (rdx == 1) {
					if (!thisProfile.powerX)
						continue;
				}
				if (rdz == -1) {
					if (!thisProfile.powerNZ)
						continue;
				}
				if (rdz == 1) {
					if (!thisProfile.powerZ)
						continue;
				}
			}

			bool canConnect = (dy == _pos.y) || RedstoneManager::CanBridgeVertical(_world, _pos, rdx, rdz, dy - _pos.y);

			if (_world.GetBlockId({ dx, dy, dz }) == BLOCK_REDSTONE && canConnect)
				if (!_visited.count({ dx, dy, dz }))
					neighbors.push_back({ dx, dy, dz });
		}
	}

	// If we had dust around us there might be dust connecting to them too
	if (neighbors.size() != 0) {
		for (auto& neighbor : neighbors) {
			GetNeighbors(_world, neighbor, _visited, _order);
		}
	}
};

static bool ResolvePowerLevels(WorldManager& _world, const std::vector<Int3>& _positions) {
	// Returns if values actually changed this time around
	bool hasChanged = false;
	for (auto& pos : _positions) {
		auto oldLevel = _world.GetMetadata(pos);
		auto newLevel = GetDustPowerLevel(_world, pos);
		if (oldLevel != newLevel) {
			_world.SetBlock(pos, BLOCK_REDSTONE, newLevel, false, false);
			hasChanged = true;
		}
	}
	return hasChanged;
}

// The end of BlockRedstoneWire.calculateCurrentChanges. Java's check is "old == 0 || (new - 1) == 0",
// so a wire that ends at 1 also counts
static void MarkWireChanged(Int3 _wire, uint8_t _oldLevel, uint8_t _newLevel) {
	if (_oldLevel != 0 && _newLevel > 1)
		return;
	blocksNeedingUpdate.Add(_wire);
	blocksNeedingUpdate.Add({ _wire.x - 1, _wire.y, _wire.z });
	blocksNeedingUpdate.Add({ _wire.x + 1, _wire.y, _wire.z });
	blocksNeedingUpdate.Add({ _wire.x, _wire.y - 1, _wire.z });
	blocksNeedingUpdate.Add({ _wire.x, _wire.y + 1, _wire.z });
	blocksNeedingUpdate.Add({ _wire.x, _wire.y, _wire.z - 1 });
	blocksNeedingUpdate.Add({ _wire.x, _wire.y, _wire.z + 1 });
}

// The end of updateAndPropagateCurrentStrength. Copy out in HashSet iteration order and clear
// before notifying, since the notifications re-enter us (Java does the same)
static void DispatchWireUpdates(WorldManager& _world) {
	const std::vector<Int3> toNotify = blocksNeedingUpdate.DrainInIterationOrder();
	for (const Int3& pos : toNotify)
		_world.NotifyNeighborsOfUpdate(pos, BLOCK_REDSTONE);
}

// Flood fill solver
// Avoids a ton of redundant updates!
static void SolveRedstoneNetwork(WorldManager& _world, Int3 _pos) {
	std::unordered_set<Int3> visited;
	std::vector<Int3> order;
	GetNeighbors(_world, _pos, visited, order, /*disable profile checks=*/true);

	std::unordered_map<Int3, uint8_t> oldLevels;
	for (auto& pos : order) {
		oldLevels.insert({ pos, static_cast<uint8_t>(_world.GetMetadata(pos)) });
	}

	while (ResolvePowerLevels(_world, order))
		;

	std::unordered_set<Int3> changed;
	for (auto& pos : order) {
		if (oldLevels.find(pos)->second != _world.GetMetadata(pos))
			changed.insert(pos);
	}
	if (changed.empty())
		return;

	// Java fills blocksNeedingUpdate from inside calculateCurrentChanges, which recurses into
	// neighboring wires before it adds its own entries. Walk the changed wires the same way
	// (post-order, same neighbor order) so the insertion order matches as closely as we can
	// without running Java's algorithm.
	std::unordered_set<Int3> walked;
	auto visit = [&](auto& _self, Int3 _wire) -> void {
		walked.insert(_wire);

		static constexpr int DX[4] = { -1, 1, 0, 0 };
		static constexpr int DZ[4] = { 0, 0, -1, 1 };
		for (int i = 0; i < 4; i++) {
			Int3 side = { _wire.x + DX[i], _wire.y, _wire.z + DZ[i] };
			if (changed.contains(side) && !walked.contains(side))
				_self(_self, side);

			// Java's second loop goes up a level beside solid blocks, otherwise down
			Int3 vertical = { side.x, side.y + (_world.IsBlockNormalCube(side) ? 1 : -1), side.z };
			if (changed.contains(vertical) && !walked.contains(vertical))
				_self(_self, vertical);
		}

		MarkWireChanged(_wire, oldLevels.find(_wire)->second, _world.GetMetadata(_wire));
	};

	if (changed.contains(_pos))
		visit(visit, _pos);
	for (auto& pos : order) {
		if (changed.contains(pos) && !walked.contains(pos))
			visit(visit, pos);
	}

	DispatchWireUpdates(_world);
}

// One BlockRedstoneWire.updateAndPropagateCurrentStrength batch, run exactly the way Java runs it
// but against an in-memory copy of the wires it touches.
//
// Java's recursion decides which wires change in a batch, in what order they're added to
// blocksNeedingUpdate, and sometimes leaves a network in a half-updated state that later
// updates depend on. None of that can be recovered from the final levels, so we run the
// same recursion. What we skip is everything that makes it slow in Java: there are no world
// writes, notifications or client updates mid-cascade, each wire's final level is written
// once, and wires are only loaded from the world when the recursion first reaches them.
namespace {
struct JavaWireBatch {
	static constexpr int STEP_BUDGET = 1 << 16;

	struct Node {
		Int3 pos;
		bool isWire = true;
		bool expanded = false;
		bool external = false; // isBlockIndirectlyGettingPowered with wires not providing power
		uint8_t level = 0;
		uint8_t originalLevel = 0;
		// Per direction (x-1, x+1, z-1, z+1): wire indices, or -1
		int side[4] = { -1, -1, -1, -1 };     // read and recursed into
		int readDiag[4] = { -1, -1, -1, -1 }; // read: up a level beside solid blocks, else down
		int recurse[4] = { -1, -1, -1, -1 };  // recursed into: up beside solid blocks, else down
	};

	WorldManager& world;
	std::deque<Node> nodes; // deque so references stay valid while we add nodes
	std::unordered_map<Int3, int> index;
	int steps = 0;
	bool overBudget = false;

	explicit JavaWireBatch(WorldManager& _world) : world(_world) {}

	// Returns the node index for a wire, or -1 if there's no wire here
	int WireAt(Int3 _pos) {
		if (auto it = index.find(_pos); it != index.end())
			return nodes[size_t(it->second)].isWire ? it->second : -1;
		if (world.GetBlockId(_pos) != BLOCK_REDSTONE)
			return -1;
		return AddNode(_pos, true, world.GetMetadata(_pos));
	}

	int AddNode(Int3 _pos, bool _isWire, uint8_t _level) {
		Node node;
		node.pos = _pos;
		node.isWire = _isWire;
		node.level = node.originalLevel = _level;
		nodes.push_back(node);
		const int i = int(nodes.size() - 1);
		index[_pos] = i;
		return i;
	}

	void Expand(int _i) {
		if (nodes[size_t(_i)].expanded)
			return;
		const Int3 p = nodes[size_t(_i)].pos;
		const bool external = DustHasExternalPower(world, p);
		const bool solidAbove = world.IsBlockNormalCube({ p.x, p.y + 1, p.z });

		static constexpr int DX[4] = { -1, 1, 0, 0 };
		static constexpr int DZ[4] = { 0, 0, -1, 1 };
		int side[4], readDiag[4], recurse[4];
		for (int d = 0; d < 4; d++) {
			const Int3 s = { p.x + DX[d], p.y, p.z + DZ[d] };
			const bool solidSide = world.IsBlockNormalCube(s);
			side[d] = WireAt(s);
			if (solidSide && !solidAbove)
				readDiag[d] = WireAt({ s.x, s.y + 1, s.z });
			else if (!solidSide)
				readDiag[d] = WireAt({ s.x, s.y - 1, s.z });
			else
				readDiag[d] = -1;
			recurse[d] = WireAt({ s.x, s.y + (solidSide ? 1 : -1), s.z });
		}

		Node& n = nodes[size_t(_i)];
		n.expanded = true;
		n.external = external;
		for (int d = 0; d < 4; d++) {
			n.side[d] = side[d];
			n.readDiag[d] = readDiag[d];
			n.recurse[d] = recurse[d];
		}
	}

	// BlockRedstoneWire.calculateCurrentChanges. _source is the node that triggered this one,
	// and is skipped when reading neighbor strengths
	void Calculate(int _i, int _source) {
		if (overBudget || ++steps > STEP_BUDGET) {
			overBudget = true;
			return;
		}
		Expand(_i);

		const int oldLevel = nodes[size_t(_i)].level;
		int newLevel = 0;
		if (nodes[size_t(_i)].external) {
			newLevel = 15;
		} else {
			const Node& n = nodes[size_t(_i)];
			for (int d = 0; d < 4; d++) {
				if (n.side[d] >= 0 && n.side[d] != _source)
					newLevel = std::max<int>(newLevel, nodes[size_t(n.side[d])].level);
				if (n.readDiag[d] >= 0 && n.readDiag[d] != _source)
					newLevel = std::max<int>(newLevel, nodes[size_t(n.readDiag[d])].level);
			}
			newLevel = newLevel > 0 ? newLevel - 1 : 0;
		}

		if (oldLevel == newLevel)
			return;
		nodes[size_t(_i)].level = uint8_t(newLevel);

		int levelBelowUs = 0;
		for (int d = 0; d < 4; d++) {
			for (int other : { nodes[size_t(_i)].side[d], nodes[size_t(_i)].recurse[d] }) {
				levelBelowUs = nodes[size_t(_i)].level;
				if (levelBelowUs > 0)
					--levelBelowUs;
				if (other >= 0 && nodes[size_t(other)].level != levelBelowUs)
					Calculate(other, _i);
				if (overBudget)
					return;
			}
		}

		// Uses whatever the strength is now, after the recursion
		levelBelowUs = nodes[size_t(_i)].level;
		if (levelBelowUs > 0)
			--levelBelowUs;
		if (oldLevel == 0 || levelBelowUs == 0) {
			const Int3 p = nodes[size_t(_i)].pos;
			blocksNeedingUpdate.Add(p);
			blocksNeedingUpdate.Add({ p.x - 1, p.y, p.z });
			blocksNeedingUpdate.Add({ p.x + 1, p.y, p.z });
			blocksNeedingUpdate.Add({ p.x, p.y - 1, p.z });
			blocksNeedingUpdate.Add({ p.x, p.y + 1, p.z });
			blocksNeedingUpdate.Add({ p.x, p.y, p.z - 1 });
			blocksNeedingUpdate.Add({ p.x, p.y, p.z + 1 });
		}
	}

	// Returns false if the cascade blew the step budget and nothing was applied
	bool Run(Int3 _pos, bool _isWire, uint8_t _level) {
		const int root = AddNode(_pos, _isWire, _level);
		Calculate(root, root);
		if (overBudget) {
			blocksNeedingUpdate.DrainInIterationOrder(); // discard
			return false;
		}

		// Java writes each step with editingBlocks set, so nothing is notified until the batch is done
		for (const Node& n : nodes) {
			if (n.isWire && n.level != n.originalLevel)
				world.SetBlock(n.pos, BLOCK_REDSTONE, n.level, false, false);
		}
		DispatchWireUpdates(world);
		return true;
	}
};
} // namespace

// BlockRedstoneWire.notifyWireNeighborsOfNeighborChange
static void NotifyWireNeighborsOfNeighborChange(WorldManager& _world, Int3 _pos) {
	if (_world.GetBlockId(_pos) != BLOCK_REDSTONE)
		return;
	_world.NotifyNeighborsOfUpdate(_pos, BLOCK_REDSTONE);
	_world.NotifyNeighborsOfUpdate({ _pos.x - 1, _pos.y, _pos.z }, BLOCK_REDSTONE);
	_world.NotifyNeighborsOfUpdate({ _pos.x + 1, _pos.y, _pos.z }, BLOCK_REDSTONE);
	_world.NotifyNeighborsOfUpdate({ _pos.x, _pos.y, _pos.z - 1 }, BLOCK_REDSTONE);
	_world.NotifyNeighborsOfUpdate({ _pos.x, _pos.y, _pos.z + 1 }, BLOCK_REDSTONE);
	_world.NotifyNeighborsOfUpdate({ _pos.x, _pos.y - 1, _pos.z }, BLOCK_REDSTONE);
	_world.NotifyNeighborsOfUpdate({ _pos.x, _pos.y + 1, _pos.z }, BLOCK_REDSTONE);
}

// The tail of BlockRedstoneWire.onBlockAdded / onBlockRemoval
static void NotifyWireFanOut(WorldManager& _world, Int3 _pos) {
	static constexpr int DX[4] = { -1, 1, 0, 0 };
	static constexpr int DZ[4] = { 0, 0, -1, 1 };
	for (int i = 0; i < 4; i++)
		NotifyWireNeighborsOfNeighborChange(_world, { _pos.x + DX[i], _pos.y, _pos.z + DZ[i] });
	for (int i = 0; i < 4; i++) {
		Int3 side = { _pos.x + DX[i], _pos.y, _pos.z + DZ[i] };
		NotifyWireNeighborsOfNeighborChange(_world,
		                                    { side.x, side.y + (_world.IsBlockNormalCube(side) ? 1 : -1), side.z });
	}
}

// Java's neighbor-of-neighbor orders. These run from onBlockAdded / onBlockRemoval, which is
// before SetBlock notifies the direct neighbors.
static constexpr Int3 TORCH_FANOUT_ORDER[6] = { { 0, -1, 0 }, { 0, 1, 0 },  { -1, 0, 0 },
	                                            { 1, 0, 0 },  { 0, 0, -1 }, { 0, 0, 1 } };
static constexpr Int3 REPEATER_FANOUT_ORDER[6] = { { 1, 0, 0 },  { -1, 0, 0 }, { 0, 0, 1 },
	                                               { 0, 0, -1 }, { 0, -1, 0 }, { 0, 1, 0 } };

void RedstoneManager::TriggerRedstoneUpdate(WorldManager& _world, Int3 _pos, BlockType _newBlock, BlockType _oldBlock,
                                            uint8_t _oldMeta) {
	// DUST:
	if (_newBlock == BLOCK_REDSTONE) {
		// BlockRedstoneWire.onBlockAdded: propagate first, then fan out
		RefreshWireAt(_world, _pos);
		_world.NotifyNeighborsOfUpdate({ _pos.x, _pos.y + 1, _pos.z }, BLOCK_REDSTONE);
		_world.NotifyNeighborsOfUpdate({ _pos.x, _pos.y - 1, _pos.z }, BLOCK_REDSTONE);
		NotifyWireFanOut(_world, _pos);
	} else if (_oldBlock == BLOCK_REDSTONE) {
		// BlockRedstoneWire.onBlockRemoval: vertical updates come before the propagation
		_world.NotifyNeighborsOfUpdate({ _pos.x, _pos.y + 1, _pos.z }, BLOCK_REDSTONE);
		_world.NotifyNeighborsOfUpdate({ _pos.x, _pos.y - 1, _pos.z }, BLOCK_REDSTONE);
		// Java runs this while the removed wire's strength is still in the metadata, so the
		// air left behind acts as the root of the batch
		if (!JavaWireBatch(_world).Run(_pos, /*isWire=*/false, _oldMeta))
			SolveRedstoneNetwork(_world, _pos);
		NotifyWireFanOut(_world, _pos);
	}

	// Torches, repeaters, levers, buttons and plates don't solve anything up front. In Java a wire
	// only recalculates when an update actually reaches it, and the order those updates arrive in
	// decides what nearby pistons see.

	// REDSTONE TORCH: onBlockAdded when it turns on, onBlockRemoval when it turns off
	if (_newBlock == BLOCK_REDSTONE_TORCH_ON || _oldBlock == BLOCK_REDSTONE_TORCH_ON) {
		for (const Int3& o : TORCH_FANOUT_ORDER)
			_world.NotifyNeighborsOfUpdate(_pos + o, BLOCK_REDSTONE_TORCH_ON);
	}

	// REDSTONE REPEATER: onBlockAdded, which runs for every on/off swap
	if (_newBlock == BLOCK_REDSTONE_REPEATER_ON || _newBlock == BLOCK_REDSTONE_REPEATER_OFF) {
		for (const Int3& o : REPEATER_FANOUT_ORDER)
			_world.NotifyNeighborsOfUpdate(_pos + o, _newBlock);
	}
}

void RedstoneManager::RefreshWireAt(WorldManager& _world, Int3 _pos) {
	// Cheap early out: if this wire's own strength wouldn't change, Java does nothing at all
	const uint8_t oldLevel = _world.GetMetadata(_pos);
	if (uint8_t(GetDustPowerLevel(_world, _pos)) == oldLevel)
		return;

	// Run Java's batch. If a cascade is too big (the kind that lags vanilla), solve it in one pass
	// instead. Levels still come out right, but the update order is only approximated then.
	if (!JavaWireBatch(_world).Run(_pos, /*isWire=*/true, oldLevel))
		SolveRedstoneNetwork(_world, _pos);
}

bool RedstoneManager::IsRepeaterInputPowered(WorldManager& _world, Int3 _pos, uint8_t _meta) {
	int facing = _meta & 3;
	switch (facing) {
	case 0: {
		Int3 inputPos = { _pos.x, _pos.y, _pos.z + 1 };
		auto block = _world.GetBlockId(inputPos);
		auto meta = _world.GetMetadata(inputPos);
		if ((block == BLOCK_REDSTONE_TORCH_ON || block == BLOCK_REDSTONE_REPEATER_ON || block == BLOCK_LEVER ||
		     block == BLOCK_BUTTON_STONE || block == BLOCK_PRESSURE_PLATE_STONE || block == BLOCK_PRESSURE_PLATE_WOOD || block == BLOCK_RAIL_DETECTOR) &&
		    RedstoneManager::GetComponentProfile(block, meta).powerNZ)
			return true;
		if (RedstoneManager::GetBlockPowerProfile(_world, inputPos).powered)
			return true;
		return block == BLOCK_REDSTONE && meta > 0;
	}
	case 1: {
		Int3 inputPos = { _pos.x - 1, _pos.y, _pos.z };
		auto block = _world.GetBlockId(inputPos);
		auto meta = _world.GetMetadata(inputPos);
		if ((block == BLOCK_REDSTONE_TORCH_ON || block == BLOCK_REDSTONE_REPEATER_ON || block == BLOCK_LEVER ||
		     block == BLOCK_BUTTON_STONE || block == BLOCK_PRESSURE_PLATE_STONE || block == BLOCK_PRESSURE_PLATE_WOOD ||
		     block == BLOCK_RAIL_DETECTOR) &&
		    RedstoneManager::GetComponentProfile(block, meta).powerX)
			return true;
		if (RedstoneManager::GetBlockPowerProfile(_world, inputPos).powered)
			return true;
		return block == BLOCK_REDSTONE && meta > 0;
	}
	case 2: {
		Int3 inputPos = { _pos.x, _pos.y, _pos.z - 1 };
		auto block = _world.GetBlockId(inputPos);
		auto meta = _world.GetMetadata(inputPos);
		if ((block == BLOCK_REDSTONE_TORCH_ON || block == BLOCK_REDSTONE_REPEATER_ON || block == BLOCK_LEVER ||
		     block == BLOCK_BUTTON_STONE || block == BLOCK_PRESSURE_PLATE_STONE || block == BLOCK_PRESSURE_PLATE_WOOD ||
		     block == BLOCK_RAIL_DETECTOR) &&
		    RedstoneManager::GetComponentProfile(block, meta).powerZ)
			return true;
		if (RedstoneManager::GetBlockPowerProfile(_world, inputPos).powered)
			return true;
		return block == BLOCK_REDSTONE && meta > 0;
	}
	case 3: {
		Int3 inputPos = { _pos.x + 1, _pos.y, _pos.z };
		auto block = _world.GetBlockId(inputPos);
		auto meta = _world.GetMetadata(inputPos);
		if ((block == BLOCK_REDSTONE_TORCH_ON || block == BLOCK_REDSTONE_REPEATER_ON || block == BLOCK_LEVER ||
		     block == BLOCK_BUTTON_STONE || block == BLOCK_PRESSURE_PLATE_STONE || block == BLOCK_PRESSURE_PLATE_WOOD ||
		     block == BLOCK_RAIL_DETECTOR) &&
		    RedstoneManager::GetComponentProfile(block, meta).powerNX)
			return true;
		if (RedstoneManager::GetBlockPowerProfile(_world, inputPos).powered)
			return true;
		return block == BLOCK_REDSTONE && meta > 0;
	}
	default:
		return false;
	}
}