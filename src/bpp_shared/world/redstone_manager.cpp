/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "redstone_manager.h"
#include "logger/logger.h"
#include "world.h"
#include <unordered_set>

bool RedstoneManager::CanBridgeVertical(WorldManager& _world, Int3 _pos, int _dx, int _dz, int _dyOffset) {
	bool sideIsSolid = Blocks::blockProperties[_world.GetBlockId({ _pos.x + _dx, _pos.y, _pos.z + _dz })].isNormalCube;

	if (_dyOffset > 0) {
		bool openAboveUs = !Blocks::blockProperties[_world.GetBlockId({ _pos.x, _pos.y + 1, _pos.z })].isNormalCube;
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
			                                     neighborBlock == BLOCK_REDSTONE);

			bool continuesHere = neighborBlock == BLOCK_REDSTONE || neighborBlock == BLOCK_REDSTONE_TORCH_ON ||
			                     neighborBlock == BLOCK_REDSTONE_TORCH_OFF;

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

PowerProfile RedstoneManager::GetBlockPowerProfile(WorldManager& _world, Int3 _pos) {
	auto thisBlock = _world.GetBlockId(_pos);

	// Non opaque blocks cant have power travel through them
	if (!Blocks::blockProperties[thisBlock].isOpaqueCube)
		return {};

	bool softPowered = false;

	// Check below us
	if (_world.GetBlockId({ _pos.x, _pos.y - 1, _pos.z }) == BLOCK_REDSTONE_TORCH_ON) {
		return { true, true };
	}

	// Check above us
	if (_world.GetBlockId({ _pos.x, _pos.y + 1, _pos.z }) == BLOCK_REDSTONE &&
	    _world.GetMetadata({ _pos.x, _pos.y + 1, _pos.z }) > 0) {
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

		// This is redstone dust, see if it is connecting to this block
		if (_world.GetBlockId(thisPos) == BLOCK_REDSTONE && _world.GetMetadata(thisPos) > 0) {
			auto grdc = GetRedstoneDustConnectivity(_world, thisPos);
			if (rdx == 1 || rdx == -1) {
				if (grdc.powerX || grdc.powerNX) {
					bool redirected = grdc.powerNZ || grdc.powerZ;
					if (!redirected)
						softPowered = true;
				}
			} else if (rdz == 1 || rdz == -1) {
				if (grdc.powerZ || grdc.powerNZ) {
					bool redirected = grdc.powerNX || grdc.powerX;
					if (!redirected)
						softPowered = true;
				}
			}
			if (!grdc.powerNX && !grdc.powerNZ && !grdc.powerX && !grdc.powerZ)
				softPowered = true;
		}

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

static int GetDustPowerLevel(WorldManager& _world, Int3 _pos) {
	auto thisBlock = _world.GetBlockId(_pos);
	auto thisMeta = _world.GetMetadata(_pos);

	// Is the block under us being hard powered?
	if (RedstoneManager::GetBlockPowerProfile(_world, _pos.WithOffset(Direction::Value::Down)).hardPowered)
		return 15;

	// Is the block above us being hard powered?
	if (RedstoneManager::GetBlockPowerProfile(_world, _pos.WithOffset(Direction::Value::Up)).hardPowered)
		return 15;

	// Can the block above us power us?
	if (RedstoneManager::GetComponentProfile(_world.GetBlockId(_pos.WithOffset(Direction::Value::Up)),
	                                         _world.GetMetadata(_pos.WithOffset(Direction::Value::Up)))
	        .powerBelow)
		return 15;

	// Check the blocks to the side of us
	int best = 0;
	int d[4] = { -1, 1, 0, 0 };
	for (int i = 0; i < 4; i++) {
		auto rdx = d[i];
		auto rdz = d[3 - i];
		auto dx = rdx + _pos.x;
		auto dz = rdz + _pos.z;

		for (int dy = _pos.y - 1; dy <= _pos.y + 1; dy++) {
			Int3 dPos = { dx, dy, dz };
			// We only care if we are being hard powered from the same Y level
			if (dy == _pos.y && RedstoneManager::GetBlockPowerProfile(_world, dPos).hardPowered)
				return 15;

			// A torch sitting directly beside us
			if (dy == _pos.y && _world.GetBlockId(dPos) == BLOCK_REDSTONE_TORCH_ON) {
				auto torchMeta = _world.GetMetadata(dPos);
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
					return 15;
			}

			// A repeater sitting directly beside us
			if (dy == _pos.y && _world.GetBlockId(dPos) == BLOCK_REDSTONE_REPEATER_ON) {
				auto repeaterMeta = _world.GetMetadata(dPos);
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
					return 15;
			}

			// Check if we are a valid connection (same level, or a valid vertical bridge)
			bool canConnect = (dy == _pos.y) || RedstoneManager::CanBridgeVertical(_world, _pos, rdx, rdz, dy - _pos.y);
			if (_world.GetBlockId(dPos) == BLOCK_REDSTONE && canConnect) {
				// This is a dust so use its current power level directly
				auto neighborLevel = _world.GetMetadata(dPos);
				if (neighborLevel > best)
					best = neighborLevel;
			}
		}
	}

	return best > 0 ? best - 1 : 0;
}

static void GetNeighbors(WorldManager& _world, Int3 _pos, std::unordered_set<Int3>& _visited) {
	auto thisBlock = _world.GetBlockId(_pos);
	bool doProfileCheck = false;

	// Only mark visited if we are redstone dust
	if (thisBlock == BLOCK_REDSTONE)
		_visited.insert(_pos);

	ComponentProfile thisProfile;
	if (thisBlock != BLOCK_REDSTONE) {
		// Make the profile getter use the powered repeater since the unpowered repeater will return false for every direction
		thisProfile = RedstoneManager::GetComponentProfile(
		    thisBlock == BLOCK_REDSTONE_REPEATER_OFF ? BLOCK_REDSTONE_REPEATER_ON : thisBlock, _world.GetMetadata(_pos));
		doProfileCheck = true;
	}

	std::unordered_set<Int3> neighbors;
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
					neighbors.insert({ dx, dy, dz });
		}
	}

	// If we had dust around us there might be dust connecting to them too
	if (neighbors.size() != 0) {
		for (auto& neighbor : neighbors) {
			GetNeighbors(_world, neighbor, _visited);
		}
	}
};

static bool ResolvePowerLevels(WorldManager& _world, std::unordered_set<Int3>& _positions) {
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

// Flood fill solver
// Avoids a ton of redundant updates!
static void SolveRedstoneNetwork(WorldManager& _world, Int3 _pos) {
	std::unordered_set<Int3> visited;
	GetNeighbors(_world, _pos, visited);

	std::unordered_map<Int3, uint8_t> oldLevels;
	for (auto& pos : visited) {
		oldLevels.insert({ pos, static_cast<uint8_t>(_world.GetMetadata(pos)) });
	}

	while (ResolvePowerLevels(_world, visited))
		;

	// Each redstone wire will try and reach further out if it went from 0->powered or powered->0
	for (auto& pos : visited) {
		auto oldLevel = oldLevels.find(pos)->second;
		auto newLevel = _world.GetMetadata(pos);
		if (oldLevel != newLevel && (oldLevel == 0 || newLevel == 0)) {
			// Always notify this wire tile's own direct neighbors
			_world.NotifyNeighborsOfUpdate(pos, BLOCK_REDSTONE);

			// Reach one hop further
			Int3 neighbors[6] = {
				{ pos.x - 1, pos.y, pos.z }, { pos.x + 1, pos.y, pos.z }, { pos.x, pos.y - 1, pos.z },
				{ pos.x, pos.y + 1, pos.z }, { pos.x, pos.y, pos.z - 1 }, { pos.x, pos.y, pos.z + 1 },
			};
			for (auto& npos : neighbors) {
				if (_world.GetBlockId(npos) != BLOCK_REDSTONE) {
					_world.NotifyNeighborsOfUpdate(npos, BLOCK_REDSTONE);
				}
			}
		}
	}
}

void RedstoneManager::TriggerRedstoneUpdate(WorldManager& _world, Int3 _pos, BlockType _newBlock, BlockType _oldBlock) {
	// Vanilla has some weird update quirks so we replicate that here in one pass
	// This avoids vanilla's tendency to spam block updates
	SolveRedstoneNetwork(_world, _pos);

	// DUST:
	if (_newBlock == BLOCK_REDSTONE || _oldBlock == BLOCK_REDSTONE) {
		// Vertical updates happen no matter what
		_world.NotifyNeighborsOfUpdate({ _pos.x, _pos.y - 1, _pos.z }, BLOCK_REDSTONE);
		_world.NotifyNeighborsOfUpdate({ _pos.x, _pos.y + 1, _pos.z }, BLOCK_REDSTONE);

		auto notifyWireNeighbor = [&_world](Int3 _neighborPos) -> void {
			if (_world.GetBlockId(_neighborPos) != BLOCK_REDSTONE)
				return;

			_world.NotifyNeighborsOfUpdate(_neighborPos, BLOCK_REDSTONE); // the neighbor itself
			_world.NotifyNeighborsOfUpdate({ _neighborPos.x - 1, _neighborPos.y, _neighborPos.z }, BLOCK_REDSTONE);
			_world.NotifyNeighborsOfUpdate({ _neighborPos.x + 1, _neighborPos.y, _neighborPos.z }, BLOCK_REDSTONE);
			_world.NotifyNeighborsOfUpdate({ _neighborPos.x, _neighborPos.y - 1, _neighborPos.z }, BLOCK_REDSTONE);
			_world.NotifyNeighborsOfUpdate({ _neighborPos.x, _neighborPos.y + 1, _neighborPos.z }, BLOCK_REDSTONE);
			_world.NotifyNeighborsOfUpdate({ _neighborPos.x, _neighborPos.y, _neighborPos.z - 1 }, BLOCK_REDSTONE);
			_world.NotifyNeighborsOfUpdate({ _neighborPos.x, _neighborPos.y, _neighborPos.z + 1 }, BLOCK_REDSTONE);
		};

		notifyWireNeighbor({ _pos.x - 1, _pos.y, _pos.z });
		notifyWireNeighbor({ _pos.x + 1, _pos.y, _pos.z });
		notifyWireNeighbor({ _pos.x, _pos.y, _pos.z - 1 });
		notifyWireNeighbor({ _pos.x, _pos.y, _pos.z + 1 });
	}

	// REDSTONE TORCH:
	if (_newBlock == BLOCK_REDSTONE_TORCH_ON || _oldBlock == BLOCK_REDSTONE_TORCH_ON) {
		Int3 sixNeighbors[6] = {
			{ _pos.x - 1, _pos.y, _pos.z }, { _pos.x + 1, _pos.y, _pos.z }, { _pos.x, _pos.y - 1, _pos.z },
			{ _pos.x, _pos.y + 1, _pos.z }, { _pos.x, _pos.y, _pos.z - 1 }, { _pos.x, _pos.y, _pos.z + 1 },
		};
		for (auto& n : sixNeighbors)
			_world.NotifyNeighborsOfUpdate(n, BLOCK_REDSTONE_TORCH_ON);
	}

	// REDSTONE REPEATER:
	if (_newBlock == BLOCK_REDSTONE_REPEATER_ON || _newBlock == BLOCK_REDSTONE_REPEATER_OFF) {
		Int3 sixNeighbors[6] = {
			{ _pos.x - 1, _pos.y, _pos.z }, { _pos.x + 1, _pos.y, _pos.z }, { _pos.x, _pos.y - 1, _pos.z },
			{ _pos.x, _pos.y + 1, _pos.z }, { _pos.x, _pos.y, _pos.z - 1 }, { _pos.x, _pos.y, _pos.z + 1 },
		};
		for (auto& n : sixNeighbors)
			_world.NotifyNeighborsOfUpdate(n, _newBlock);
	}
}

void RedstoneManager::RefreshWireAt(WorldManager& _world, Int3 _pos) {
	// Redstone wires on neighbor change gets called independently of whether the update was caused by a redstone component
	SolveRedstoneNetwork(_world, _pos);
}

bool RedstoneManager::IsRepeaterInputPowered(WorldManager& _world, Int3 _pos, uint8_t _meta) {
	int facing = _meta & 3;
	switch (facing) {
	case 0: {
		Int3 inputPos = { _pos.x, _pos.y, _pos.z + 1 };
		auto block = _world.GetBlockId(inputPos);
		auto meta = _world.GetMetadata(inputPos);
		if ((block == BLOCK_REDSTONE_TORCH_ON || block == BLOCK_REDSTONE_REPEATER_ON) &&
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
		if ((block == BLOCK_REDSTONE_TORCH_ON || block == BLOCK_REDSTONE_REPEATER_ON) &&
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
		if ((block == BLOCK_REDSTONE_TORCH_ON || block == BLOCK_REDSTONE_REPEATER_ON) &&
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
		if ((block == BLOCK_REDSTONE_TORCH_ON || block == BLOCK_REDSTONE_REPEATER_ON) &&
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