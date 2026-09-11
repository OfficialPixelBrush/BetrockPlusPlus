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

static bool CanRedstoneComponentStay(WorldManager& _world, Int3 _pos) {
	return _world.IsBlockNormalCube(_pos.Offset(Direction::Value::Down));
}

static void NotifyAttachedSupportBlock(WorldManager& _world, Int3 _pos, BlockType _blockId, uint8_t _meta) {
	Direction::Value dir = GetDirectionFromMeta(_blockId, _meta);
	Int3 support = _pos.WithOffset(Direction::Opposite(dir));

	static constexpr Direction::Value ALL_DIRS[6] = {
		Direction::Value::North, Direction::Value::South, Direction::Value::East,
		Direction::Value::West,  Direction::Value::Up,    Direction::Value::Down,
	};
	for (auto d : ALL_DIRS) {
		if (d == dir)
			continue;
		Int3 neighborPos = support.WithOffset(d);
		auto block = _world.GetBlockId(neighborPos);
		auto updateFunction = Blocks::blockBehaviors[block].onNeighborBlockChange;
		if (updateFunction)
			updateFunction(_world, neighborPos, _blockId);
	}
}

void RegisterRedstoneBehaviors() {
	// Redstone dust
	blockBehaviors[BlockType::BLOCK_REDSTONE] = {
		.getSelectionBox = RedstoneDustAabb,
		.getRayBounds = RedstoneDustAabb,
		.getCollider = EmptyCollider,
		.onNeighborBlockChange = [](WorldManager& _world, Int3 _pos, BlockType _blockId) -> void {
		    if (_blockId != BLOCK_REDSTONE)
			    RedstoneManager::RefreshWireAt(_world, _pos);
		    if (!CanRedstoneComponentStay(_world, _pos)) {
			    BreakAndDropBlock(_world, _pos);
		    }
		},
	};

	blockBehaviors[BlockType::BLOCK_REDSTONE_REPEATER_OFF] = {
		.getSelectionBox = RepeaterAabb,
		.getRayBounds = RepeaterAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_REDSTONE_REPEATER_ON] = {
		.getSelectionBox = RepeaterAabb,
		.getRayBounds = RepeaterAabb,
		.getCollider = EmptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_BUTTON_STONE] = {
		.getSelectionBox = ButtonAabb,
		.getRayBounds = ButtonAabb,
		.getCollider = EmptyCollider,
		.onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta, Java::Random& /*_random*/) -> void {
		    // Check to make sure we can till exist here
		    if (!IsSupported(_world, _pos, GetDirectionFromMeta(BLOCK_BUTTON_STONE, _meta)))
			    BreakAndDropBlock(_world, _pos);
		    // Unpress again
		    if (_meta & 0b1000) {
			    _world.SetMeta(_pos, _meta & 0b111);
			    NotifyAttachedSupportBlock(_world, _pos, BLOCK_BUTTON_STONE, _meta);
		    }
		},
		.onNeighborBlockChange = [](WorldManager& _world, Int3 _pos, BlockType /*_blockId*/) -> void {
		    blockBehaviors[BLOCK_BUTTON_STONE].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
		},
		.onBlockClicked = [](WorldManager& _world, Int3 _pos, PlayerSession* _triggeringSession) -> void {
		    auto newMeta = _world.GetMetadata(_pos) | 0b1000;
		    _world.SetMeta(_pos, newMeta);
		    NotifyAttachedSupportBlock(_world, _pos, BLOCK_BUTTON_STONE, newMeta);
		    _world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_BUTTON_STONE, 20);
		    if (_world.onWorldEvent)
			    _world.onWorldEvent(PacketData::WorldEvent::CLICK2, _pos, 0, _triggeringSession);
		},
		.onBlockActivated = [](WorldManager& _world, Int3 _pos, PlayerSession* _triggeringSession) -> bool {
		    blockBehaviors[BLOCK_BUTTON_STONE].onBlockClicked(_world, _pos, _triggeringSession);
		    return false;
		},
		.onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& /*_placer*/, Direction::Value _face,
		                    BlockType _blockId, uint8_t /*_meta*/) -> bool {
		    // Buttons can only be placed against the sides of blocks
		    if (_face == Direction::Value::Up || _face == Direction::Value::Down)
			    return false;
		    if (!IsReplaceable(_world, _pos))
			    return false;
		    _world.SetBlock(_pos, _blockId, GetMetaFromDirection(BLOCK_BUTTON_STONE, _face));
		    return true;
		},
	};

	blockBehaviors[BlockType::BLOCK_LEVER] = {
		.getRayBounds = LeverAabb,
		.getCollider = EmptyCollider,
		// getSelectionBox stays defaultAABB
	};

	blockBehaviors[BlockType::BLOCK_PRESSURE_PLATE_STONE] = {
		.getSelectionBox = PressurePlateAabb,
		.getRayBounds = PressurePlateAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_PRESSURE_PLATE_WOOD] = {
		.getSelectionBox = PressurePlateAabb,
		.getRayBounds = PressurePlateAabb,
		.getCollider = EmptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_TORCH] = {
		.getSelectionBox = TorchAabb,
		.getRayBounds = TorchAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_REDSTONE_TORCH_OFF] = {
		.getSelectionBox = TorchAabb,
		.getRayBounds = TorchAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_REDSTONE_TORCH_ON] = {
		.getSelectionBox = TorchAabb,
		.getRayBounds = TorchAabb,
		.getCollider = EmptyCollider,
	};

	blockBehaviors[BLOCK_REDSTONE_TORCH_ON].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                                           Direction::Value _face, BlockType _blockId,
	                                                           uint8_t /*_meta*/) -> bool {
		if (_world.GetBlockId(_pos.WithOffset(Direction::Opposite(_face))) == BLOCK_SNOW_LAYER)
			_pos = _pos.WithOffset(Direction::Opposite(_face));
		if (CanTorchAttachTo(_world, _pos, _face)) {
			return GenericPlace(_world, _pos, _placer, _face, _blockId,
			                    GetMetaFromDirection(BLOCK_REDSTONE_TORCH_ON, _face));
		}
		return false;
	};

	blockBehaviors[BLOCK_REDSTONE_TORCH_ON].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		const auto currentFace = GetDirectionFromMeta(BLOCK_REDSTONE_TORCH_ON, _world.GetMetadata(_pos));
		if (CanTorchAttachTo(_world, _pos, currentFace))
			return; // Already valid

		// Matches vanilla order
		static constexpr std::array<Direction::Value, 5> CHECK_ORDER = { Direction::Value::East, Direction::Value::West,
			                                                             Direction::Value::South,
			                                                             Direction::Value::North, Direction::Value::Up };

		// Attach to the first support block we find
		for (Direction::Value face : CHECK_ORDER) {
			if (CanTorchAttachTo(_world, _pos, face)) {
				_world.SetMeta(_pos, GetMetaFromDirection(BLOCK_REDSTONE_TORCH_ON, face));
				return;
			}
		}

		BreakAndDropBlock(_world, _pos);
	};

	blockBehaviors[BLOCK_REDSTONE_TORCH_ON].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                                   BlockType /*_blockId*/) -> void {
		auto dir = GetDirectionFromMeta(BLOCK_REDSTONE_TORCH_ON, _world.GetMetadata(_pos));
		if (!CanTorchAttachTo(_world, _pos, dir)) {
			BreakAndDropBlock(_world, _pos);
			return;
		}

		// Torches take two ticks to update
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_REDSTONE_TORCH_ON, 2);
	};

	blockBehaviors[BLOCK_REDSTONE_TORCH_ON].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                                    Java::Random& /*_random*/) -> void {
		const auto dir = GetDirectionFromMeta(BLOCK_REDSTONE_TORCH_ON, _meta);
		if (!CanTorchAttachTo(_world, _pos, dir)) {
			BreakAndDropBlock(_world, _pos);
			return;
		}

		Int3 supportPos = _pos.WithOffset(Direction::Opposite(dir));

		// turn OFF the instant the block it's mounted on is powered
		if (RedstoneManager::GetBlockPowerProfile(_world, supportPos).powered) {
			_world.SetBlock(_pos, BLOCK_REDSTONE_TORCH_OFF, _meta);
		}
	};

	blockBehaviors[BLOCK_REDSTONE_TORCH_OFF].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		const auto dir = GetDirectionFromMeta(BLOCK_REDSTONE_TORCH_OFF, _world.GetMetadata(_pos));
		if (CanTorchAttachTo(_world, _pos, dir))
			return; // Already valid

		static constexpr std::array<Direction::Value, 5> CHECK_ORDER = { Direction::Value::East, Direction::Value::West,
			                                                             Direction::Value::South,
			                                                             Direction::Value::North, Direction::Value::Up };

		for (Direction::Value face : CHECK_ORDER) {
			if (CanTorchAttachTo(_world, _pos, face)) {
				_world.SetMeta(_pos, GetMetaFromDirection(BLOCK_REDSTONE_TORCH_OFF, face));
				return;
			}
		}

		BreakAndDropBlock(_world, _pos);
	};

	blockBehaviors[BLOCK_REDSTONE_TORCH_OFF].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                                    BlockType /*_blockId*/) -> void {
		const auto dir = GetDirectionFromMeta(BLOCK_REDSTONE_TORCH_OFF, _world.GetMetadata(_pos));
		if (!CanTorchAttachTo(_world, _pos, dir)) {
			BreakAndDropBlock(_world, _pos);
			return;
		}

		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_REDSTONE_TORCH_OFF, 2);
	};

	blockBehaviors[BLOCK_REDSTONE_TORCH_OFF].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                                     Java::Random& /*_random*/) -> void {
		const auto dir = GetDirectionFromMeta(BLOCK_REDSTONE_TORCH_OFF, _meta);
		if (!CanTorchAttachTo(_world, _pos, dir)) {
			BreakAndDropBlock(_world, _pos);
			return;
		}

		Int3 supportPos = _pos.WithOffset(Direction::Opposite(dir));

		// An unlit torch turns back ON the instant the block it's mounted on is no longer powered
		if (!RedstoneManager::GetBlockPowerProfile(_world, supportPos).powered) {
			_world.SetBlock(_pos, BLOCK_REDSTONE_TORCH_ON, _meta);
		}
	};

	blockBehaviors[BLOCK_REDSTONE_REPEATER_OFF].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                                               Direction::Value _face, BlockType _blockId,
	                                                               uint8_t /*_meta*/) -> bool {
		const auto dir = Direction::FromAngle(_placer.rotationYaw);
		auto meta = GetMetaFromDirection(BLOCK_REDSTONE_REPEATER_OFF, dir);
		if (!CanRedstoneComponentStay(_world, _pos) || !GenericPlace(_world, _pos, _placer, _face, _blockId, meta))
			return false;
		// Turn on if we are being powered!
		if (RedstoneManager::IsRepeaterInputPowered(_world, _pos, meta)) {
			_world.tickScheduler.ScheduleUpdateTick(_pos, _blockId, 1);
		}
		return true;
	};

	blockBehaviors[BLOCK_REDSTONE].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                                  Direction::Value _face, BlockType _blockId,
	                                                  uint8_t _meta) -> bool {
		if (!CanRedstoneComponentStay(_world, _pos) || !GenericPlace(_world, _pos, _placer, _face, _blockId, _meta))
			return false;
		return true;
	};

	blockBehaviors[BLOCK_REDSTONE_REPEATER_OFF].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                                       BlockType /*_blockId*/) -> void {
		if (!CanRedstoneComponentStay(_world, _pos)) {
			BreakAndDropBlock(_world, _pos);
			return;
		}
		auto meta = _world.GetMetadata(_pos);
		auto thisBlock = _world.GetBlockId(_pos);
		bool inputPowered = RedstoneManager::IsRepeaterInputPowered(_world, _pos, meta);
		bool isPoweredRepeater = _world.GetBlockId(_pos) ==
		                         BLOCK_REDSTONE_REPEATER_ON; // Powered repeater calls this function too
		int delaySetting = (meta & 12) >> 2;
		int delayTicks = (delaySetting + 1) * 2;

		if (isPoweredRepeater && !inputPowered) {
			_world.tickScheduler.ScheduleUpdateTick(_pos, thisBlock, delayTicks);
		} else if (!isPoweredRepeater && inputPowered) {
			_world.tickScheduler.ScheduleUpdateTick(_pos, thisBlock, delayTicks);
		}
	};

	blockBehaviors[BLOCK_REDSTONE_REPEATER_OFF].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                                        Java::Random& /*_random*/) -> void {
		bool inputPowered = RedstoneManager::IsRepeaterInputPowered(_world, _pos, _meta);
		bool isPoweredRepeater = _world.GetBlockId(_pos) ==
		                         BLOCK_REDSTONE_REPEATER_ON; // Powered repeater calls this function too

		if (isPoweredRepeater && !inputPowered) {
			// Turn off if we are no longer being powered
			_world.SetBlock(_pos, BLOCK_REDSTONE_REPEATER_OFF, _meta);
		} else if (!isPoweredRepeater) {
			// Turn on unconditionally
			_world.SetBlock(_pos, BLOCK_REDSTONE_REPEATER_ON, _meta);
			if (!inputPowered) {
				// If we are no longer being powered request another update
				int delaySetting = (_meta & 12) >> 2;
				int delayTicks = (delaySetting + 1) * 2;
				_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_REDSTONE_REPEATER_ON, delayTicks);
			}
		}
	};

	blockBehaviors[BLOCK_REDSTONE_REPEATER_ON].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                                      BlockType _blockId) -> void {
		blockBehaviors[BLOCK_REDSTONE_REPEATER_OFF].onNeighborBlockChange(_world, _pos, _blockId);
	};

	blockBehaviors[BLOCK_REDSTONE_REPEATER_ON].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                                       Java::Random& _random) -> void {
		blockBehaviors[BLOCK_REDSTONE_REPEATER_OFF].onTick(_world, _pos, _meta, _random);
	};

	blockBehaviors[BLOCK_REDSTONE_REPEATER_ON].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                                              Direction::Value _face, BlockType _blockId,
	                                                              uint8_t _meta) -> bool {
		return blockBehaviors[BLOCK_REDSTONE_REPEATER_OFF].onBlockPlaced(_world, _pos, _placer, _face, _blockId, _meta);
	};

	blockBehaviors[BLOCK_LEVER].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                               Direction::Value _face, BlockType _blockId, uint8_t /*_meta*/) -> bool {
		if (_world.GetBlockId(_pos.WithOffset(Direction::Opposite(_face))) == BLOCK_SNOW_LAYER)
			_pos = _pos.WithOffset(Direction::Opposite(_face));
		// TODO: This isn't exactly the best way to do it, as it relies on Metadata fuckery,
		// but it works, and it's better than bastardizing the Direction system we got!
		bool isEastWestAligned = false;
		if (_face == Direction::Value::Up) {
			auto alignment = Direction::FromAngle(_placer.rotationYaw);
			if (alignment == Direction::Value::East || alignment == Direction::Value::West)
				isEastWestAligned = true;
		}
		if (CanTorchAttachTo(_world, _pos, _face)) {
			return GenericPlace(_world, _pos, _placer, _face, _blockId,
			                    GetMetaFromDirection(BLOCK_LEVER, _face) + isEastWestAligned);
		}
		return false;
	};

	blockBehaviors[BLOCK_LEVER].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		const auto currentFace = GetDirectionFromMeta(BLOCK_LEVER, _world.GetMetadata(_pos));
		if (CanTorchAttachTo(_world, _pos, currentFace))
			return; // Already valid

		// Matches vanilla order
		static constexpr std::array<Direction::Value, 5> CHECK_ORDER = { Direction::Value::East, Direction::Value::West,
			                                                             Direction::Value::South,
			                                                             Direction::Value::North, Direction::Value::Up };

		// Attach to the first support block we find
		for (Direction::Value face : CHECK_ORDER) {
			if (CanTorchAttachTo(_world, _pos, face)) {
				_world.SetMeta(_pos, GetMetaFromDirection(BLOCK_LEVER, face));
				return;
			}
		}

		BreakAndDropBlock(_world, _pos);
	};

	blockBehaviors[BLOCK_LEVER].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos, BlockType /*_blockId*/) -> void {
		auto dir = GetDirectionFromMeta(BLOCK_LEVER, _world.GetMetadata(_pos));
		if (!CanTorchAttachTo(_world, _pos, dir)) {
			BreakAndDropBlock(_world, _pos);
			return;
		}
	};

	blockBehaviors[BLOCK_LEVER].onBlockClicked = [](WorldManager& _world, Int3 _pos,
	                                                PlayerSession* _triggeringSession) -> void {
		auto newMeta = _world.GetMetadata(_pos) ^ 0b1000;
		_world.SetMeta(_pos, newMeta);
		NotifyAttachedSupportBlock(_world, _pos, BLOCK_LEVER, newMeta);
		if (_world.onWorldEvent)
			_world.onWorldEvent(PacketData::WorldEvent::CLICK2, _pos, 0, _triggeringSession);
	},
	blockBehaviors[BLOCK_LEVER].onBlockActivated = [](WorldManager& _world, Int3 _pos,
	                                                  PlayerSession* _triggeringSession) -> bool {
		blockBehaviors[BLOCK_LEVER].onBlockClicked(_world, _pos, _triggeringSession);
		return false;
	},

	blockBehaviors[BLOCK_TORCH].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                               Direction::Value _face, BlockType _blockId, uint8_t /*_meta*/) -> bool {
		if (_world.GetBlockId(_pos.WithOffset(Direction::Opposite(_face))) == BLOCK_SNOW_LAYER)
			_pos = _pos.WithOffset(Direction::Opposite(_face));
		if (CanTorchAttachTo(_world, _pos, _face)) {
			return GenericPlace(_world, _pos, _placer, _face, _blockId, GetMetaFromDirection(BLOCK_TORCH, _face));
		}
		return false;
	};

	blockBehaviors[BLOCK_TORCH].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		const auto currentFace = GetDirectionFromMeta(BLOCK_TORCH, _world.GetMetadata(_pos));
		if (CanTorchAttachTo(_world, _pos, currentFace))
			return; // Already valid

		// Matches vanilla order
		static constexpr std::array<Direction::Value, 5> CHECK_ORDER = { Direction::Value::East, Direction::Value::West,
			                                                             Direction::Value::South,
			                                                             Direction::Value::North, Direction::Value::Up };

		// Attach to the first support block we find
		for (Direction::Value face : CHECK_ORDER) {
			if (CanTorchAttachTo(_world, _pos, face)) {
				_world.SetMeta(_pos, GetMetaFromDirection(BLOCK_TORCH, face));
				return;
			}
		}

		BreakAndDropBlock(_world, _pos);
	};

	blockBehaviors[BLOCK_TORCH].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos, BlockType /*_blockId*/) -> void {
		const auto dir = GetDirectionFromMeta(BLOCK_TORCH, _world.GetMetadata(_pos));
		if (!CanTorchAttachTo(_world, _pos, dir))
			BreakAndDropBlock(_world, _pos);
	};

	// for when the block is interacted with!
	blockBehaviors[BLOCK_REDSTONE_REPEATER_OFF].onBlockActivated = [](WorldManager& _world, Int3 _pos,
	                                                                  PlayerSession* /*_triggeringSession*/) -> bool {
		// Increase and loop delay
		auto meta = _world.GetMetadata(_pos);
		int delay = (meta & 12) >> 2;
		delay = (delay + 1) << 2 & 12;
		_world.SetMeta(_pos, delay | (meta & 3));
		return false;
	};
	blockBehaviors[BLOCK_REDSTONE_REPEATER_ON].onBlockActivated = [](WorldManager& _world, Int3 _pos,
	                                                                 PlayerSession* _triggeringSession) -> bool {
		return blockBehaviors[BLOCK_REDSTONE_REPEATER_OFF].onBlockActivated(_world, _pos, _triggeringSession);
	};
}

}; // namespace Blocks
