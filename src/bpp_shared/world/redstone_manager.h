/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "../helpers/direction_fixer.h"
#include "base_structs.h"
#include "blocks.h"
#include "blocks/block_behaviors.h"
#include "blocks/block_properties_behaviors.h"

// For redstone!
struct WorldManager;
struct ComponentProfile {
	// These are for purely whether we can power other components / dust
	// NOT if we can power that block!
	bool powerX : 1 = false;
	bool powerNX : 1 = false;
	bool powerZ : 1 = false;
	bool powerNZ : 1 = false;
	bool powerBelow : 1 = false;
};

struct PowerProfile {
	bool powered : 1 = false;
	bool hardPowered : 1 = false;
};

namespace RedstoneManager {
void TriggerRedstoneUpdate(WorldManager& _world, Int3 _pos, BlockType _newBlock, BlockType _oldBlock);

// Call this from redstone dust's onNeighborBlockChange
void RefreshWireAt(WorldManager& _world, Int3 _pos);

bool CanBridgeVertical(WorldManager& _world, Int3 _pos, int _dx, int _dz, int _dyOffset);
bool IsRepeaterInputPowered(WorldManager& _world, Int3 _pos, uint8_t _meta);
ComponentProfile GetRedstoneDustConnectivity(WorldManager& _world, Int3 _pos);
PowerProfile GetBlockPowerProfile(WorldManager& _world, Int3 _pos);

static bool CanProvidePower(BlockType _block) {
	// Repeaters are excluded for some reason in vanilla
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

static bool CanTriggerRedstoneUpdate(BlockType _block) {
	switch (_block) {
	case BLOCK_REDSTONE:
	case BLOCK_LEVER:
	case BLOCK_BUTTON_STONE:
	case BLOCK_PRESSURE_PLATE_STONE:
	case BLOCK_PRESSURE_PLATE_WOOD:
	case BLOCK_REDSTONE_TORCH_ON:
	case BLOCK_REDSTONE_TORCH_OFF:
	case BLOCK_REDSTONE_REPEATER_ON:
	case BLOCK_REDSTONE_REPEATER_OFF:
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
		switch (GetDirectionFromMeta(BLOCK_REDSTONE_TORCH_ON, _meta)) {
		case Direction::Value::North:
			return ComponentProfile{ true, true, false, true, true };
		case Direction::Value::South:
			return ComponentProfile{ true, true, true, false, true };
		case Direction::Value::East:
			return ComponentProfile{ true, false, true, true, true };
		case Direction::Value::West:
			return ComponentProfile{ false, true, true, true, true };
		case Direction::Value::Up:
			return ComponentProfile{ true, true, true, true, false };
		default:
			return {};
		}
	}
	case BLOCK_REDSTONE_TORCH_OFF: {
		return {};
	}
	case BLOCK_REDSTONE_REPEATER_ON: {
		switch (GetDirectionFromMeta(BLOCK_REDSTONE_REPEATER_ON, _meta)) {
		case Direction::Value::North:
			return ComponentProfile{ false, false, false, true, false }; // outputs -Z
		case Direction::Value::East:
			return ComponentProfile{ true, false, false, false, false }; // outputs +X
		case Direction::Value::South:
			return ComponentProfile{ false, false, true, false, false }; // outputs +Z
		case Direction::Value::West:
			return ComponentProfile{ false, true, false, false, false }; // outputs -X
		default:
			return {};
		}
	}
	case BLOCK_REDSTONE_REPEATER_OFF: {
		return {};
	}
	case BLOCK_LEVER: {
		if (!(_meta & 0b1000))
			return {};
		switch (GetDirectionFromMeta(BLOCK_LEVER, _meta)) {
		case Direction::Value::North:
			return ComponentProfile{ true, true, false, true, true };
		case Direction::Value::South:
			return ComponentProfile{ true, true, true, false, true };
		case Direction::Value::East:
			return ComponentProfile{ true, false, true, true, true };
		case Direction::Value::West:
			return ComponentProfile{ false, true, true, true, true };
		case Direction::Value::Up:
			return ComponentProfile{ true, true, true, true, false };
		default:
			return {};
		}
	}
	case BLOCK_BUTTON_STONE: {
		if (!(_meta & 0b1000))
			return {};
		switch (GetDirectionFromMeta(BLOCK_BUTTON_STONE, _meta)) {
		case Direction::Value::North:
			return ComponentProfile{ true, true, false, true, true };
		case Direction::Value::South:
			return ComponentProfile{ true, true, true, false, true };
		case Direction::Value::East:
			return ComponentProfile{ true, false, true, true, true };
		case Direction::Value::West:
			return ComponentProfile{ false, true, true, true, true };
		case Direction::Value::Up:
			return ComponentProfile{ true, true, true, true, false };
		default:
			return {};
		}
	}
	default: {
		return {};
	}
	}
}
}; // namespace RedstoneManager