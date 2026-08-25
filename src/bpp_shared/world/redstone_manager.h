/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "base_structs.h"
#include "blocks.h"
#include "blocks/block_behaviors.h"
#include "blocks/block_properties_behaviors.h"

// For redstone!
struct WorldManager;
struct ComponentProfile {
	// These are for purely whether we can power other components / dust
	// NOT if we can power that block!
	bool powerX = false;
	bool powerNX = false;
	bool powerZ = false;
	bool powerNZ = false;
	bool powerBelow = false;
};

struct PowerProfile {
	bool powered = false;
	bool hardPowered = false;
};

namespace RedstoneManager {
void TriggerRedstoneUpdate(WorldManager& _world, Int3 _pos, BlockType _newBlock, BlockType _oldBlock);

// Call this from redstone dust's onNeighborBlockChange
void RefreshWireAt(WorldManager& _world, Int3 _pos);

bool CanBridgeVertical(WorldManager& _world, Int3 _pos, int _dx, int _dz, int _dyOffset);
bool IsRepeaterInputPowered(WorldManager& _world, Int3 _pos, uint8_t _meta);
ComponentProfile GetRedstoneDustConnectivity(WorldManager& _world, Int3 _pos);
PowerProfile GetBlockPowerProfile(WorldManager& _world, Int3 _pos);

static bool CanTriggerRedstoneUpdate(BlockType _block) {
	switch (_block) {
	case BLOCK_REDSTONE:
	case BLOCK_LEVER:
	case BLOCK_BUTTON_STONE:
	case BLOCK_PRESSURE_PLATE_STONE:
	case BLOCK_PRESSURE_PLATE_WOOD:
	case BLOCK_REDSTONE_TORCH_ON:
	case BLOCK_REDSTONE_TORCH_OFF:
		return true;
	default:
		return false;
	}
}

static ComponentProfile GetComponentProfile(BlockType _blockId, uint8_t _meta) {
	switch (_blockId) {
	case BLOCK_REDSTONE: {
		return ComponentProfile{ true, true, true, true, false };
	}
	case BLOCK_REDSTONE_TORCH_ON: {
		if (_meta == 1) {
			// Facing X+1
			return ComponentProfile{ true, false, true, true, true };
		} else if (_meta == 2) {
			// Facing X-1
			return ComponentProfile{ false, true, true, true, true };
		} else if (_meta == 3) {
			// Facing Z+1
			return ComponentProfile{ true, true, true, false, true };
		} else if (_meta == 4) {
			// Facing Z-1
			return ComponentProfile{ true, true, false, true, true };
		} else if (_meta == 5) {
			// On floor
			return ComponentProfile{ true, true, true, true, false };
		} else {
			// Invalid
			return {};
		}
	}
	case BLOCK_REDSTONE_TORCH_OFF: {
		return {};
	}
	case BLOCK_REDSTONE_REPEATER_ON: {
		switch (_meta & 0b11) {
		case 0:
			return ComponentProfile{ false, false, false, true, false }; // outputs -Z
		case 1:
			return ComponentProfile{ true, false, false, false, false }; // outputs +X
		case 2:
			return ComponentProfile{ false, false, true, false, false }; // outputs +Z
		case 3:
			return ComponentProfile{ false, true, false, false, false }; // outputs -X
		default:
			return {};
		}
	}
	case BLOCK_REDSTONE_REPEATER_OFF: {
		return {};
	}
	default: {
		return {};
	}
	}
}
}; // namespace RedstoneManager