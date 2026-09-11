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

static void TryTallPlantGrowth(WorldManager& _world, Int3 _pos, uint8_t _meta, BlockType _block) {
	if (_world.GetBlockId(_pos.WithOffset(Direction::Value::Up)) != BLOCK_AIR)
		return;

	bool canContinue = true;
	uint8_t height = 1;
	Int3 checkPos = _pos;
	while (canContinue) {
		if (height >= 3)
			return;
		if (_world.GetBlockId(checkPos.Offset(Direction::Value::Down)) == _block) {
			height++;
			continue;
		}
		canContinue = false;
	}
	if (_meta >= 15) {
		_world.SetBlock(_pos.WithOffset(Direction::Value::Up), _block);

		// So we dont regrown instantly
		// Also updates the new plant block
		_world.SetMeta(_pos, 0);
		return;
	}
	_world.SetMeta(_pos, _meta + 1);
}

static float GetCropGrowthRate(WorldManager& _world, Int3 _pos) {
	float rate = 1.0f;

	// Check each axis to see if there is wheat there
	BlockType north = _world.GetBlockId(_pos.WithOffset(Direction::Value::North));
	BlockType south = _world.GetBlockId(_pos.WithOffset(Direction::Value::South));
	BlockType west = _world.GetBlockId(_pos.WithOffset(Direction::Value::West));
	BlockType east = _world.GetBlockId(_pos.WithOffset(Direction::Value::East));
	BlockType nw = _world.GetBlockId(_pos.WithOffset(Direction::Value::North).Offset(Direction::Value::West));
	BlockType ne = _world.GetBlockId(_pos.WithOffset(Direction::Value::North).Offset(Direction::Value::East));
	BlockType se = _world.GetBlockId(_pos.WithOffset(Direction::Value::South).Offset(Direction::Value::East));
	BlockType sw = _world.GetBlockId(_pos.WithOffset(Direction::Value::South).Offset(Direction::Value::West));

	const bool wheatOnXAxis = (west == BLOCK_CROP_WHEAT || east == BLOCK_CROP_WHEAT);
	const bool wheatOnZAxis = (north == BLOCK_CROP_WHEAT || south == BLOCK_CROP_WHEAT);
	const bool wheatDiagonal = (nw == BLOCK_CROP_WHEAT || ne == BLOCK_CROP_WHEAT || se == BLOCK_CROP_WHEAT ||
	                            sw == BLOCK_CROP_WHEAT);

	// Check the quality of the farmland around us
	// Our own tile counts fully and neighbor tiles count for 1/4
	for (int x = _pos.x - 1; x <= _pos.x + 1; x++) {
		for (int z = _pos.z - 1; z <= _pos.z + 1; z++) {
			BlockType below = _world.GetBlockId({ x, _pos.y - 1, z });
			float contribution = 0.0f;
			if (below == BLOCK_FARMLAND) {
				contribution = 1.0f;
				if (_world.GetMetadata({ x, _pos.y - 1, z }) > 0)
					contribution = 3.0f; // moist farmland scores triple

				if (x != _pos.x || z != _pos.z)
					contribution /= 4.0f; // neighbor tiles count for 1/4
			}
			rate += contribution;
		}
	}

	// If there is wheat on the diagonal or on both axes, the growth rate is halved
	if (wheatDiagonal || (wheatOnXAxis && wheatOnZAxis))
		rate /= 2.0f;

	return rate;
}

static bool SearchForLog(int _sLength, Int3 _pos, Int3 _cameFrom, WorldManager& _world) {
	auto thisBlock = _world.GetBlockId(_pos);
	if (thisBlock == BLOCK_LOG)
		return true;
	if (_sLength > 3 || thisBlock != BLOCK_LEAVES)
		return false;

	int d[4] = { -1, 1, 0, 0 };
	for (int i = 0; i < 4; i++) {
		int dx = _pos.x + d[i];
		int dz = _pos.z + d[3 - i];
		Int3 newPos = { dx, _pos.y, dz };
		if (newPos == _cameFrom)
			continue;
		if (SearchForLog(_sLength + 1, newPos, _pos, _world))
			return true;
	}

	for (int i = 0; i < 2; i++) {
		int dy = _pos.y + d[i];
		Int3 newPos = { _pos.x, dy, _pos.z };
		if (newPos == _cameFrom)
			continue;
		if (SearchForLog(_sLength + 1, newPos, _pos, _world))
			return true;
	}

	return false;
}

void RegisterPlantBehaviors() {
	// Farmland
	blockBehaviors[BlockType::BLOCK_FARMLAND] = {
		.getSelectionBox = FarmlandAabb,
		.getRayBounds = FarmlandAabb,
		.getCollider = FarmlandCollider,
	};

	// Crops
	blockBehaviors[BlockType::BLOCK_CROP_WHEAT] = {
		.getSelectionBox = CropAabb,
		.getRayBounds = CropAabb,
		.getCollider = EmptyCollider,
	};

	// Sapling
	blockBehaviors[BlockType::BLOCK_SAPLING] = {
		.getSelectionBox = SaplingAabb,
		.getRayBounds = SaplingAabb,
		.getCollider = EmptyCollider,
	};

	// Tall grass
	blockBehaviors[BlockType::BLOCK_TALLGRASS] = {
		.getSelectionBox = TallGrassAabb,
		.getRayBounds = TallGrassAabb,
		.getCollider = EmptyCollider,
	};

	// Mushrooms
	blockBehaviors[BlockType::BLOCK_MUSHROOM_BROWN] = {
		.getSelectionBox = MushroomAabb,
		.getRayBounds = MushroomAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_MUSHROOM_RED] = {
		.getSelectionBox = MushroomAabb,
		.getRayBounds = MushroomAabb,
		.getCollider = EmptyCollider,
	};

	// Flowers (rose, dandelion)
	blockBehaviors[BlockType::BLOCK_ROSE] = {
		.getSelectionBox = PlantAabb,
		.getRayBounds = PlantAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_DANDELION] = {
		.getSelectionBox = PlantAabb,
		.getRayBounds = PlantAabb,
		.getCollider = EmptyCollider,
	};

	// Dead bush
	blockBehaviors[BlockType::BLOCK_DEADBUSH] = {
		.getSelectionBox = SaplingAabb, // same f=0.4 box as sapling
		.getRayBounds = SaplingAabb,
		.getCollider = EmptyCollider,
	};

	// Sugar cane
	blockBehaviors[BlockType::BLOCK_SUGARCANE] = {
		.getSelectionBox = SugarcaneAabb,
		.getRayBounds = SugarcaneAabb,
		.getCollider = EmptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_CACTUS] = {
		.getSelectionBox = CactusAabb,
		.getRayBounds = CactusAabb,
		.getCollider = CactusCollider,
	};

	blockBehaviors[BLOCK_CACTUS].onEntityCollidedWithBlock = [](WorldManager& /*_world*/, Int3 /*_pos*/,
	                                                            Entity& _entity) -> void {
		_entity.AttackEntityFrom(nullptr, 1);
	};
	blockBehaviors[BLOCK_CROP_WHEAT].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                            BlockType /*_blockId*/) -> void {
		if (!CanCropsSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};

	blockBehaviors[BLOCK_CROP_WHEAT].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                             Java::Random& _random) -> void {
		if (_world.GetBlockLightValue({ _pos.x, _pos.y + 1, _pos.z }) < 9)
			return;
		if (_meta >= 7)
			return;
		auto rate = GetCropGrowthRate(_world, _pos);
		if (_random.NextInt(int(100.0f / rate)) == 0)
			_world.SetMeta(_pos, _meta + 1);
	};
	blockBehaviors[BLOCK_SUGARCANE].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                           BlockType /*_blockId*/) -> void {
		// Check to see if our placement is still valid
		if (!CanSugarcaneSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_SUGARCANE].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                            Java::Random& /*_random*/) -> void {
		TryTallPlantGrowth(_world, _pos, _meta, BLOCK_SUGARCANE);
	};

	auto onPlantPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face, BlockType _blockId,
	                       uint8_t _meta) -> bool {
		if (CanGenericPlantSurviveAt(_world, _pos)) {
			return GenericPlace(_world, _pos, _placer, _face, _blockId, _meta);
		}
		return false;
	};

	auto onMushroomPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face,
	                          BlockType _blockId, uint8_t _meta) -> bool {
		if (CanMushroomSurviveAt(_world, _pos)) {
			return GenericPlace(_world, _pos, _placer, _face, _blockId, _meta);
		}
		return false;
	};

	auto onCactusPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face,
	                        BlockType _blockId, uint8_t _meta) -> bool {
		if (CanCactusSurviveAt(_world, _pos)) {
			return GenericPlace(_world, _pos, _placer, _face, _blockId, _meta);
		}
		return false;
	};

	// Farmland
	blockBehaviors[BLOCK_FARMLAND].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                          BlockType /*_blockId*/) -> void {
		// Revert to dirt if something is solid above us
		if (_world.GetMaterial({ _pos.x, _pos.y + 1, _pos.z }).isSolid) {
			_world.SetBlock(_pos, BLOCK_DIRT, 0);
		}
	};
	blockBehaviors[BLOCK_FARMLAND].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                           Java::Random& _random) -> void {
		if (_random.NextInt(5) != 0)
			return;

		// Check if we are hydrated
		auto waterNearby = [](WorldManager& _farmWorld, Int3 _farmPos) -> bool {
			for (int dx = -4; dx <= 4; dx++) {
				for (int dz = -4; dz <= 4; dz++) {
					for (int dy = 0; dy <= 1; dy++) {
						Int3 checkPos = { _farmPos.x + dx, _farmPos.y + dy, _farmPos.z + dz };
						if (_farmWorld.GetMaterial(checkPos) == Material::Water())
							return true;
					}
				}
			}
			return false;
		};

		// There is water so max hydration
		if (waterNearby(_world, _pos)) {
			if (_meta < 7) {
				_world.SetMeta(_pos, 7);
			}
			return;
		}

		// We still have room to dry out
		if (_meta > 0) {
			_world.SetMeta(_pos, _meta - 1);
			return;
		}

		// We are completely dry, check if we have a crop on top of us
		if (_world.GetBlockId({ _pos.x, _pos.y + 1, _pos.z }) != BLOCK_CROP_WHEAT)
			_world.SetBlock(_pos, BLOCK_DIRT, 0);
	};

	// Plants
	blockBehaviors[BLOCK_CACTUS].onBlockPlaced = onCactusPlace;
	blockBehaviors[BLOCK_MUSHROOM_BROWN].onBlockPlaced = onMushroomPlace;
	blockBehaviors[BLOCK_MUSHROOM_RED].onBlockPlaced = onMushroomPlace;
	blockBehaviors[BLOCK_DANDELION].onBlockPlaced = onPlantPlace;
	blockBehaviors[BLOCK_ROSE].onBlockPlaced = onPlantPlace;
	blockBehaviors[BLOCK_SAPLING].onBlockPlaced = onPlantPlace;
	blockBehaviors[BLOCK_TALLGRASS].onBlockPlaced = onPlantPlace;
	blockBehaviors[BLOCK_CACTUS].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                        BlockType /*_blockId*/) -> void {
		if (!CanCactusSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_MUSHROOM_BROWN].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                                BlockType /*_blockId*/) -> void {
		blockBehaviors[BLOCK_MUSHROOM_BROWN].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_MUSHROOM_RED].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                              BlockType /*_blockId*/) -> void {
		blockBehaviors[BLOCK_MUSHROOM_RED].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_DANDELION].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                           BlockType /*_blockId*/) -> void {
		blockBehaviors[BLOCK_DANDELION].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_ROSE].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                      BlockType /*_blockId*/) -> void {
		blockBehaviors[BLOCK_ROSE].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_TALLGRASS].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                           BlockType /*_blockId*/) -> void {
		blockBehaviors[BLOCK_TALLGRASS].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_SAPLING].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                         BlockType /*_blockId*/) -> void {
		if (!CanGenericPlantSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_DANDELION].onTick = [](WorldManager& _world, Int3 _pos, uint8_t /*_meta*/,
	                                            Java::Random& /*_random*/) -> void {
		if (!CanGenericPlantSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_ROSE].onTick = [](WorldManager& _world, Int3 _pos, uint8_t /*_meta*/,
	                                       Java::Random& /*_random*/) -> void {
		if (!CanGenericPlantSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_TALLGRASS].onTick = [](WorldManager& _world, Int3 _pos, uint8_t /*_meta*/,
	                                            Java::Random& /*_random*/) -> void {
		if (!CanGenericPlantSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_MUSHROOM_BROWN].onTick = [](WorldManager& _world, Int3 _pos, uint8_t /*_meta*/,
	                                                 Java::Random& /*_random*/) -> void {
		if (!CanMushroomSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_MUSHROOM_RED].onTick = [](WorldManager& _world, Int3 _pos, uint8_t /*_meta*/,
	                                               Java::Random& /*_random*/) -> void {
		if (!CanMushroomSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_CACTUS].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                         Java::Random& /*_random*/) -> void {
		TryTallPlantGrowth(_world, _pos, _meta, BLOCK_CACTUS);
	};

	// Grass spread / decay
	blockBehaviors[BLOCK_GRASS].onTick = [](WorldManager& _world, Int3 _pos, uint8_t /*_meta*/,
	                                        Java::Random& _random) -> void {
		Int3 aboveBlockPos = { _pos.x, _pos.y + 1, _pos.z };
		auto lightLevel = _world.GetBlockLightValue(aboveBlockPos);
		auto block = _world.GetBlockId(aboveBlockPos);
		auto opacity = blockProperties[block].lightOpacity;

		if (lightLevel < 4 && opacity > 2) {
			// Decay
			if (_random.NextInt(4) != 0)
				return;
			_world.SetBlock(_pos, BLOCK_DIRT);
		} else if (lightLevel >= 9) {
			// If we have enough light try and spread
			int dx = _pos.x + _random.NextInt(3) - 1;
			int dy = _pos.y + _random.NextInt(5) - 3;
			int dz = _pos.z + _random.NextInt(3) - 1;
			Int3 abovePos = { dx, dy + 1, dz };
			auto aboveBlock = _world.GetBlockId(abovePos);
			if (_world.GetBlockId({ dx, dy, dz }) == BLOCK_DIRT && _world.GetBlockLightValue(abovePos) >= 4 &&
			    blockProperties[aboveBlock].lightOpacity <= 2) {
				_world.SetBlock({ dx, dy, dz }, BLOCK_GRASS);
			}
		}
	};

	// Leaf decay!
	blockBehaviors[BLOCK_LEAVES].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                         Java::Random& /*_random*/) -> void {
		// Are we marked to check for despawn?
		if ((_meta & 8) != 0) {
			if (!SearchForLog(0, _pos, _pos, _world)) {
				BreakAndDropBlock(_world, _pos);
				return;
			}
			_world.SetMeta(_pos, _meta & ~8);
		}
	};
	blockBehaviors[BLOCK_LEAVES].onBlockDestroyedByPlayer = [](WorldManager& _world, Int3 _pos, Entity& _destroyer) {
		// Leaves drop themselves instead of their random loot table when broken with shears
		PlayerEntity* pe = dynamic_cast<PlayerEntity*>(&_destroyer);
		if (!pe)
			return;

		auto heldItem = pe->GetHeldItem();
		if (heldItem && heldItem->id == Items::Id::SHEARS) {
			// Isolate the leaf type
			auto thisMeta = (_world.GetMetadata(_pos) & 3);
			_world.SetBlock(_pos, BLOCK_AIR);
			DropBlockAt(_world, _pos, BLOCK_LEAVES, /*count=*/1, thisMeta);
			return;
		}
		GenericBreak(_world, _pos, _destroyer);
	};
	blockBehaviors[BLOCK_LEAVES].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                                Direction::Value _face, BlockType _blockId, uint8_t _meta) -> bool {
		// So leaves placed by players dont decay
		return GenericPlace(_world, _pos, _placer, _face, _blockId, /*meta=*/_meta & 0b11);
	};
	// Leaves and logs flag leaves to check for removal
	blockBehaviors[BLOCK_LOG].onBlockRemoval = [](WorldManager& _world, Int3 _pos) -> void {
		// Mark a 9x9 area dirty if they are leaves
		int dist = 4;
		for (int x = -dist; x <= dist; x++) {
			for (int z = -dist; z <= dist; z++) {
				for (int y = -dist; y <= dist; y++) {
					auto dpos = _pos + Int3{ x, y, z };
					if (_world.GetBlockId(dpos) == BLOCK_LEAVES) {
						auto meta = _world.GetMetadata(dpos);
						if (!(meta & 8))
							_world.SetMeta(dpos, meta |= 8);
					}
				}
			}
		}
	};
	blockBehaviors[BLOCK_LEAVES].onBlockRemoval = [](WorldManager& _world, Int3 _pos) -> void {
		// Mark a 3x3 area dirty if they are leaves
		int dist = 1;
		for (int x = -dist; x <= dist; x++) {
			for (int z = -dist; z <= dist; z++) {
				for (int y = -dist; y <= dist; y++) {
					auto dpos = _pos + Int3{ x, y, z };
					if (_world.GetBlockId(dpos) == BLOCK_LEAVES) {
						auto meta = _world.GetMetadata(dpos);
						if (!(meta & 8))
							_world.SetMeta(dpos, meta |= 8);
					}
				}
			}
		}
	};
}

}; // namespace Blocks
