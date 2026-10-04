/*
 * Copyright (c) 2026, Anya Rihtarshich <vesui@proton.me>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */

#include "blocks.h"
#include "blocks/block_behaviors.h"
#include "blocks/block_properties.h"
#include "internal.h"
#include "numeric_structs.h"
#include "packet_data.h"
#include "world.h"

namespace Blocks {

bool CanBlockCatchFire(WorldManager& _world, Int3 _pos) {
	BlockType id = _world.GetBlockId(_pos);
	if (id == BLOCK_AIR)
		return false;
	if (Blocks::blockProperties[id].material.canBurn)
		return true;
	if (id == BLOCK_TALLGRASS) // tallgrass burns in vanilla.
		return true;
	return false;
}

bool CanNeighborBurn(WorldManager& _world, Int3 _pos) {
	static constexpr std::array<Direction::Value, 6> ALL_DIRS = {
	    Direction::Value::West,
		Direction::Value::East,
		Direction::Value::Down,
		Direction::Value::Up,
		Direction::Value::North,
		Direction::Value::South
	};
	for (Direction::Value dir : ALL_DIRS) {
		if (CanBlockCatchFire(_world, _pos.WithOffset(dir)))
			return true;
	}
	return false;
}

bool CanFireStay(WorldManager& _world, Int3 _pos) {
	return _world.IsBlockNormalCube(_pos.WithOffset(Direction::Value::Down))
	|| CanNeighborBurn(_world, _pos);
}

void RegisterFireBehaviors() {
	// placement
	blockBehaviors[BLOCK_FIRE].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		TryCreatePortal(_world, _pos);
		if (_world.GetBlockId(_pos) == BLOCK_FIRE && !CanFireStay(_world, _pos)) {
			_world.SetBlock(_pos, BLOCK_AIR);
		}
	};

	// update
	blockBehaviors[BLOCK_FIRE].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos, BlockType) -> void {
		if (!CanFireStay(_world, _pos)) {
			_world.SetBlock(_pos, BLOCK_AIR);
		}
	};

	// extinguish
	blockBehaviors[BLOCK_FIRE].onBlockClicked = [](WorldManager& _world, Int3 _pos, PlayerSession* _triggeringSession) -> void {
		_world.SetBlock(_pos, BLOCK_AIR);
		if (_world.onWorldEvent) {
			_world.onWorldEvent(PacketData::WorldEvent::FIRE_EXTINGUISH, _pos, 0, _triggeringSession);
		}
	};
}

}; // namespace Blocks
