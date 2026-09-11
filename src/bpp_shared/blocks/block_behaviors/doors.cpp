/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "blocks.h"
#include "blocks/block_behaviors.h"
#include "blocks/block_properties.h"
#include "dimensions.h"
#include "entities/entity_falling_block.h"
#include "entities/entity_player.h"
#include "entities/entity_skeleton.h"
#include "entities/entity_spider.h"
#include "entities/entity_zombie.h"
#include "enums/items.h"
#include "generator/overworld/tree_gen.h"
#include "helpers/direction_fixer.h"
#include "helpers/java/java_math.h"
#include "internal.h"
#include "items/item_properties.h"
#include "logger.h"
#include "numeric_structs.h"
#include "packet_data.h"
#include "rail_manager.h"
#include "redstone_manager.h"
#include "tick_scheduler.h"
#include "tile_entities/tile_entity.h"
#include "world.h"

namespace Blocks {

static void ToggleTrapdoor(WorldManager& _world, Int3 _pos) {
	auto meta = _world.GetMetadata(_pos);
	_world.SetMeta(_pos, uint8_t(meta ^ 0b100)); // XOR bit 2; flips open/closed
	return;
}

static void ToggleDoor(WorldManager& _world, Int3 _pos, PlayerSession* _triggeringSession) {
	auto meta = _world.GetMetadata(_pos);
	if (meta & 8) {
		// We are the top half of the door
		if (_world.GetBlockId({ _pos.x, _pos.y - 1, _pos.z }) != BLOCK_DOOR_WOOD)
			// Below us is not the bottom of a door! This is bad!
			return;
		// Recall this function on the bottom of the door
		blockBehaviors[BLOCK_DOOR_WOOD].onBlockActivated(_world, { _pos.x, _pos.y - 1, _pos.z }, _triggeringSession);
		return;
	}
	// We are the top half so lets open
	Int3 top = { _pos.x, _pos.y + 1, _pos.z };
	if (_world.GetBlockId(top) == BLOCK_DOOR_WOOD && (_world.GetMetadata(top) & 8)) {
		_world.SetMeta(top, uint8_t((meta ^ 0b100) + 8));
	}
	_world.SetMeta(_pos, uint8_t(meta ^ 0b100)); // XOR bit 2; flips open/closed
	if (_world.onWorldEvent)
		_world.onWorldEvent(PacketData::WorldEvent::DOOR_TOGGLE, _pos, 0, _triggeringSession);
	return;
}

static void BreakDoor(WorldManager& _world, Int3 _pos, BlockType _doorType) {
	auto meta = _world.GetMetadata(_pos);
	if (meta & 8) {
		// Offset post down
		_pos.Offset(Direction::Value::Down);
		// We are the top of the door
		if (_world.GetBlockId(_pos) != _doorType)
			// Below us is not the bottom of a door! This is bad!
			return;
	}
	// Since we're now guaranteed to be
	// pointing at the bottom of the door,
	// we can continue like this
	Int3 top = _pos.WithOffset(Direction::Value::Up);
	if (_world.GetBlockId(top) == _doorType && (_world.GetMetadata(top) & 8)) {
		_world.SetBlock(top, BLOCK_AIR);
	}
	BreakAndDropBlock(_world, _pos);
}

void RegisterDoorBehaviors() {
	blockBehaviors[BlockType::BLOCK_DOOR_WOOD] = {
		.getSelectionBox = DoorAabb,
		.getRayBounds = DoorAabb,
		.getCollider = DoorCollider,
	};
	blockBehaviors[BlockType::BLOCK_DOOR_IRON] = {
		.getSelectionBox = DoorAabb,
		.getRayBounds = DoorAabb,
		.getCollider = DoorCollider,
	};

	blockBehaviors[BlockType::BLOCK_TRAPDOOR] = {
		.getSelectionBox = TrapdoorAabb,
		.getRayBounds = TrapdoorAabb,
		.getCollider = TrapdoorCollider,
		.onBlockActivated = [](WorldManager& _world, Int3 _pos, PlayerSession* /*_triggeringSession*/) -> bool {
		    ToggleTrapdoor(_world, _pos);
		    return false;
		},
		.onBlockPlaced = [](WorldManager& _world, Int3 _pos, [[maybe_unused]] Entity& _placer, Direction::Value _face,
		                    BlockType _blockId, [[maybe_unused]] uint8_t _meta) -> bool {
		    // Doors can only be placed against the sides of blocks
		    if (_face == Direction::Value::Up || _face == Direction::Value::Down)
			    return false;

		    if (!IsReplaceable(_world, _pos))
			    return false;

		    _world.SetBlock(_pos, _blockId, GetMetaFromDirection(BLOCK_TRAPDOOR, Direction::Opposite(_face)));
		    return true;
		}
	};

	auto onDoorPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face, BlockType _blockId,
	                      [[maybe_unused]] uint8_t _meta) -> bool {
		// Doors can only be placed by clicking the top face of a block
		if (_face != Direction::Value::Up)
			return false;

		// _pos is already the target cell
		const Int3 placePos = _pos;
		const Int3 abovePos = placePos.WithOffset(Direction::Value::Up);

		// Is this placement valid?
		if (!_world.InBounds(abovePos.y))
			return false;
		if (!_world.IsBlockNormalCube(placePos.WithOffset(Direction::Value::Down)))
			return false;
		if (!IsReplaceable(_world, placePos) || !IsReplaceable(_world, abovePos))
			return false;

		auto facing = Direction::FromAngle(_placer.rotationYaw);

		const Int3 left = placePos.WithOffset(Direction::ToLeft(facing));
		const Int3 leftTop = left.WithOffset(Direction::Value::Up);

		const Int3 right = placePos.WithOffset(Direction::ToRight(facing));
		const Int3 rightTop = right.WithOffset(Direction::Value::Up);

		const int leftSolidBlocks = (_world.IsBlockNormalCube(left) ? 1 : 0) +
		                            (_world.IsBlockNormalCube(leftTop) ? 1 : 0);
		const int rightSolidBlocks = (_world.IsBlockNormalCube(right) ? 1 : 0) +
		                             (_world.IsBlockNormalCube(rightTop) ? 1 : 0);

		const bool leftHasDoor = _world.GetBlockId(left) == _blockId || _world.GetBlockId(leftTop) == _blockId;
		const bool rightHasDoor = _world.GetBlockId(right) == _blockId || _world.GetBlockId(rightTop) == _blockId;

		bool hingeOnRight = false;
		if (leftHasDoor && !rightHasDoor) {
			hingeOnRight = true;
		} else if (rightSolidBlocks > leftSolidBlocks) {
			hingeOnRight = true;
		}

		if (hingeOnRight)
			facing = Direction::ToLeft(facing);

		uint8_t meta = GetMetaFromDirection(_blockId, facing);
		if (hingeOnRight)
			meta |= 4;

		_world.SetBlock(placePos, _blockId, meta);
		_world.SetBlock(abovePos, _blockId, uint8_t(meta | 8));

		// heldItem->DecrementCount(1) is handled by the caller when this returns true.
		return true;
	};

	blockBehaviors[BLOCK_DOOR_WOOD].onBlockPlaced = onDoorPlace;
	blockBehaviors[BLOCK_DOOR_IRON].onBlockPlaced = onDoorPlace;

	blockBehaviors[BLOCK_DOOR_WOOD].onBlockActivated = [](WorldManager& _world, Int3 _pos,
	                                                      PlayerSession* _triggeringSession) -> bool {
		ToggleDoor(_world, _pos, _triggeringSession);
		return false;
	};
	blockBehaviors[BLOCK_DOOR_WOOD].onBlockClicked = ToggleDoor;
	blockBehaviors[BLOCK_DOOR_WOOD].onBlockDestroyedByPlayer = [](WorldManager& _world, Int3 _pos, [[maybe_unused]] Entity& _destroyer) {
		BreakDoor(_world, _pos, BLOCK_DOOR_WOOD);
	};
	blockBehaviors[BLOCK_DOOR_IRON].onBlockDestroyedByPlayer = [](WorldManager& _world, Int3 _pos, [[maybe_unused]] Entity& _destroyer) {
		BreakDoor(_world, _pos, BLOCK_DOOR_IRON);
	};
}

}; // namespace Blocks
