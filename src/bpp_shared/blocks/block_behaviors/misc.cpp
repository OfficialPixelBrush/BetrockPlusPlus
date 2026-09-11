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

static int GetDirectionFromYaw(float _yaw, int _directionCount) {
	return MathHelper::FloorDouble((_yaw * _directionCount / 360.0f) + 0.5f) & 3;
}

void RegisterMiscBehaviors() {
	blockBehaviors[BlockType::BLOCK_COBWEB] = {
		.getCollider = EmptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_SLAB] = {
		.getSelectionBox = SlabAabb,
		.getRayBounds = SlabAabb,
		.getCollider = SlabCollider,
	};

	blockBehaviors[BlockType::BLOCK_STAIRS_WOOD] = {
		.getCollider = StairCollider,
		// ray/selection stay as defaultAABB
	};
	blockBehaviors[BlockType::BLOCK_STAIRS_COBBLESTONE] = {
		.getCollider = StairCollider,
	};

	blockBehaviors[BlockType::BLOCK_SNOW_LAYER] = {
		.getRayBounds = SnowLayerAabb,
		.getCollider = SnowLayerCollider,
		// getSelectionBox stays defaultAABB
	};

	blockBehaviors[BlockType::BLOCK_LADDER] = {
		.getSelectionBox = LadderAabb,
		.getRayBounds = LadderAabb,
		.getCollider = LadderCollider,
	};

	blockBehaviors[BlockType::BLOCK_FENCE] = {
		.getCollider = FenceCollider,
		// ray/selection stay as defaultAABB (full cube)
	};

	blockBehaviors[BlockType::BLOCK_CAKE] = {
		.getSelectionBox = CakeAabb,
		.getRayBounds = CakeAabb,
		.getCollider = CakeCollider,
	};

	blockBehaviors[BlockType::BLOCK_PISTON_HEAD] = {
		.getSelectionBox = PistonHeadAabb,
		.getRayBounds = PistonHeadAabb,
		.getCollider = PistonHeadCollider,
	};

	blockBehaviors[BLOCK_SOULSAND] = {
		.getCollider = SoulSandCollider,
	};

	blockBehaviors[BLOCK_COBWEB].onEntityCollidedWithBlock = [](WorldManager& _world, Int3 _pos,
	                                                            Entity& _entity) -> void {
		_entity.inWeb = true;
	};
	blockBehaviors[BLOCK_SOULSAND].onEntityCollidedWithBlock = [](WorldManager& _world, Int3 _pos,
	                                                              Entity& _entity) -> void {
		_entity.velocity.x *= 0.4;
		_entity.velocity.z *= 0.4;
	};
	// Placement overrides
	auto onFurnaceDispenserPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face,
	                                  BlockType _blockId, uint8_t _meta) -> bool {
		int meta[] = { 2, 5, 3, 4 };
		return GenericPlace(_world, _pos, _placer, _face, _blockId, meta[GetDirectionFromYaw(_placer.rotationYaw, 4)]);
	};

	auto onPumpkinPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face,
	                         BlockType _blockId, uint8_t _meta) -> bool {
		int meta[] = { 2, 3, 0, 1 };
		auto belowBlockMaterial = _world.GetMaterial({ _pos.x, _pos.y - 1, _pos.z });
		if (!belowBlockMaterial.isOpaque)
			return false;
		return GenericPlace(_world, _pos, _placer, _face, _blockId, meta[GetDirectionFromYaw(_placer.rotationYaw, 4)]);
	};

	auto onStairPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face, BlockType _blockId,
	                       uint8_t _meta) -> bool {
		int meta[] = { 2, 1, 3, 0 };
		return GenericPlace(_world, _pos, _placer, _face, _blockId, meta[GetDirectionFromYaw(_placer.rotationYaw, 4)]);
	};

	auto onPistonPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face,
	                        BlockType _blockId, uint8_t _meta) -> bool {
		return GenericPlace(_world, _pos, _placer, _face, _blockId, GetMetaFromDirection(BLOCK_PISTON, _face));
	};

	blockBehaviors[BLOCK_PISTON].onBlockPlaced = onPistonPlace;
	blockBehaviors[BLOCK_PISTON_STICKY].onBlockPlaced = onPistonPlace;

	// Fence
	blockBehaviors[BLOCK_FENCE].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                               Direction::Value _face, BlockType _blockId, uint8_t _meta) -> bool {
		if (_world.GetBlockId(_pos.WithOffset(Direction::Value::Down)) == BLOCK_FENCE)
			return true;

		if (!_world.GetMaterial(_pos.WithOffset(Direction::Value::Down)).isSolid)
			return false;

		return GenericPlace(_world, _pos, _placer, _face, _blockId, _meta);
	};

	// Slabs
	blockBehaviors[BLOCK_SLAB].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                              Direction::Value _face, BlockType _blockId, uint8_t _meta) -> bool {
		auto sourcePos = _pos.WithOffset(Direction::Opposite(_face));
		auto sourceBlock = _world.GetBlockId(sourcePos);
		auto sourceMeta = _world.GetMetadata(sourcePos);
		if (sourceBlock == BLOCK_SLAB && _face == Direction::Value::Up && sourceMeta == _meta) {
			_world.SetBlock(sourcePos, BLOCK_AIR);
			return GenericPlace(_world, sourcePos, _placer, _face, BLOCK_DOUBLE_SLAB, _meta);
		}
		return GenericPlace(_world, _pos, _placer, _face, _blockId, _meta);
	};

	// Snow
	blockBehaviors[BLOCK_SNOW_LAYER].onBlockDestroyedByPlayer = [](WorldManager& _world, Int3 _pos, Entity& _destroyer) {
		// Snow drops itself when broken with a shovel
		_world.SetBlock(_pos, BLOCK_AIR);
		DropItemAt(_world, _pos, Items::Id::SNOWBALL, /*count=*/1, 0);
		return;
	};
	blockBehaviors[BLOCK_SNOW_LAYER].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                            BlockType _blockId) -> void {
		const BlockType below = _world.GetBlockId(_pos.WithOffset(Direction::Value::Down));
		const bool canStay = (below != BLOCK_AIR) && Blocks::blockProperties[below].isOpaqueCube &&
		                     Blocks::blockProperties[below].material.isSolid;
		if (!canStay)
			_world.SetBlock(_pos, BLOCK_AIR);
	};

	// Directionals
	blockBehaviors[BLOCK_PUMPKIN].onBlockPlaced = onPumpkinPlace;
	blockBehaviors[BLOCK_PUMPKIN_LIT].onBlockPlaced = onPumpkinPlace;
	blockBehaviors[BLOCK_FURNACE].onBlockPlaced = onFurnaceDispenserPlace;
	blockBehaviors[BLOCK_FURNACE_LIT].onBlockPlaced = onFurnaceDispenserPlace;
	blockBehaviors[BLOCK_DISPENSER].onBlockPlaced = onFurnaceDispenserPlace;
	blockBehaviors[BLOCK_STAIRS_COBBLESTONE].onBlockPlaced = onStairPlace;
	blockBehaviors[BLOCK_STAIRS_WOOD].onBlockPlaced = onStairPlace;
	blockBehaviors[BLOCK_LADDER].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                        BlockType _blockId) -> void {
		blockBehaviors[BLOCK_LADDER].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_LADDER].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                         Java::Random& _random) -> void {
		// Check to make sure we can till exist here
		if (!IsSupported(_world, _pos, GetDirectionFromMeta(BLOCK_LADDER, _meta)))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_LADDER].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                                Direction::Value _face, BlockType _blockId, uint8_t _meta) -> bool {
		Int3 targetPos = _pos;
		const Int3 sourceBlock = _pos.WithOffset(Direction::Opposite(_face));
		if (_world.GetBlockId(sourceBlock) == BLOCK_SNOW_LAYER)
			targetPos = sourceBlock;

		static constexpr std::array<Direction::Value, 4> CHECK_ORDER = {
			Direction::Value::North, Direction::Value::South, Direction::Value::West, Direction::Value::East
		};

		if (Direction::IsHorizontal(_face) && IsSupported(_world, _pos, _face)) {
			return GenericPlace(_world, _pos, _placer, _face, _blockId, GetMetaFromDirection(BLOCK_LADDER, _face));
		}

		for (Direction::Value dir : CHECK_ORDER) {
			if (IsSupported(_world, _pos, dir)) {
				return GenericPlace(_world, _pos, _placer, _face, _blockId, GetMetaFromDirection(BLOCK_LADDER, dir));
			}
		}
		return false;
	};

	blockBehaviors[BLOCK_FURNACE].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		auto furnaceTileEntity = std::make_shared<TileEntityFurnace>(_pos);
		_world.CreateTileEntity(std::move(furnaceTileEntity));
	};

	auto dropFurnaceInventory = [](WorldManager& _world, Int3 _pos) -> void {
		auto* te = _world.GetTileEntityAs<TileEntityFurnace>(_pos);
		if (!te)
			return;

		_world.DropInventory(te->inventory, _pos);
	};

	blockBehaviors[BLOCK_FURNACE].onBlockRemoval = dropFurnaceInventory;
	blockBehaviors[BLOCK_FURNACE_LIT].onBlockRemoval = dropFurnaceInventory;
}

}; // namespace Blocks
