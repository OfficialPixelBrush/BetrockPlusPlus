/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "redstone_manager.h"
#include "world.h"
#include <unordered_set>

// Determines whether redstone dust can bridge a one-block vertical step
static bool CanBridgeVertical(WorldManager& _world, Int3 _pos, int _dx, int _dz, int _dyOffset) {
	bool sideIsSolid = Blocks::blockProperties[_world.GetBlockId({ _pos.x + _dx, _pos.y, _pos.z + _dz })].isNormalCube;

	if (_dyOffset > 0) {
		bool openAboveUs = !Blocks::blockProperties[_world.GetBlockId({ _pos.x, _pos.y + 1, _pos.z })].isNormalCube;
		return sideIsSolid && openAboveUs;
	}

	return !sideIsSolid;
}

static ComponentProfile GetRedstoneDustConnectivity(WorldManager& _world, Int3 _pos) {
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

			bool canConnect = (dy == _pos.y) || CanBridgeVertical(_world, _pos, rdx, rdz, dy - _pos.y);

			if (_world.GetBlockId({ dx, dy, dz }) == BLOCK_REDSTONE && canConnect) {
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

static PowerProfile GetBlockPowerProfile(WorldManager& _world, Int3 _pos) {
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
			auto GRDC = GetRedstoneDustConnectivity(_world, thisPos);
			if (rdx != 0) {
				// We care about the X here
				if (!GRDC.powerX && !GRDC.powerNX)
					softPowered = true;
			}
			if (rdz != 0) {
				// We care about the Z here
				if (!GRDC.powerZ && !GRDC.powerNZ)
					softPowered = true;
			}
		}

		// This is a repeater, see if it is facing us and powered
		if (_world.GetBlockId(thisPos) == BLOCK_REDSTONE_REPEATER_ON) {
			// stub
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
	if (GetBlockPowerProfile(_world, { _pos.x, _pos.y - 1, _pos.z }).hardPowered)
		return 15;

	// Is the block above us being hard powered?
	if (GetBlockPowerProfile(_world, { _pos.x, _pos.y + 1, _pos.z }).hardPowered)
		return 15;

	// Can the block above us power us?
	if (RedstoneManager::GetComponentProfile(_world.GetBlockId({ _pos.x, _pos.y + 1, _pos.z }),
	                                         _world.GetMetadata({ _pos.x, _pos.y + 1, _pos.z }))
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
			// We only care if we are being hard powered from the same Y level
			if (dy == _pos.y && GetBlockPowerProfile(_world, { dx, dy, dz }).hardPowered)
				return 15;

			// A torch sitting directly beside us
			if (dy == _pos.y && _world.GetBlockId({ dx, dy, dz }) == BLOCK_REDSTONE_TORCH_ON) {
				auto torchMeta = _world.GetMetadata({ dx, dy, dz });
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

			// Check if we are a valid connection (same level, or a valid vertical bridge)
			bool canConnect = (dy == _pos.y) || CanBridgeVertical(_world, _pos, rdx, rdz, dy - _pos.y);
			if (_world.GetBlockId({ dx, dy, dz }) == BLOCK_REDSTONE && canConnect) {
				// This is a dust - use its current power level directly
				auto neighborLevel = _world.GetMetadata({ dx, dy, dz });
				if (neighborLevel > best)
					best = neighborLevel;
			}
		}
	}

	return best > 0 ? best - 1 : 0;
}

static void GetNeighbors(WorldManager& _world, Int3 _pos, std::unordered_set<Int3>& _visited) {
	if (_world.GetBlockId(_pos) == BLOCK_REDSTONE)
		_visited.insert(_pos);

	std::unordered_set<Int3> neighbors;
	// Horizontal scan
	for (int dy = -1 + _pos.y; dy <= 1 + _pos.y; dy++) {
		int d[4] = { -1, 1, 0, 0 };
		for (int i = 0; i < 4; i++) {
			auto rdx = d[i];
			auto rdz = d[3 - i];
			auto dx = rdx + _pos.x;
			auto dz = rdz + _pos.z;

			bool canConnect = (dy == _pos.y) || CanBridgeVertical(_world, _pos, rdx, rdz, dy - _pos.y);

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

void RedstoneManager::TriggerRedstoneUpdate(WorldManager& _world, Int3 _pos) {
	// Recursive scan for redstone dust
	std::unordered_set<Int3> visited;
	GetNeighbors(_world, _pos, visited);

	// Resolve our power levels
	while (ResolvePowerLevels(_world, visited));
}