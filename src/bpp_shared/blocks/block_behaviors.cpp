/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "blocks/block_behaviors.h"
#include "../helpers/direction_fixer.h"
#include "../helpers/java/java_math.h"
#include "../numeric_structs.h"
#include "blocks.h"
#include "blocks/block_properties.h"
#include "dimensions.h"
#include "entities/entity_falling_block.h"
#include "entities/entity_player.h"
#include "enums/items.h"
#include "generator/overworld/tree_gen.h"
#include "items/item_properties.h"
#include "logger.h"
#include "packet_data.h"
#include "redstone_manager.h"
#include "tick_scheduler.h"
#include "tile_entities/tile_entity.h"
#include "world.h"

namespace Blocks {
BlockBehavior blockBehaviors[BLOCK_MAX] = {};

static float GetCropGrowthRate(WorldManager& _world, Int3 _pos) {
	float rate = 1.0f;

	// Check each axis to see if there is wheat there
	BlockType north = _world.GetBlockId({ _pos.x, _pos.y, _pos.z - 1 });
	BlockType south = _world.GetBlockId({ _pos.x, _pos.y, _pos.z + 1 });
	BlockType west = _world.GetBlockId({ _pos.x - 1, _pos.y, _pos.z });
	BlockType east = _world.GetBlockId({ _pos.x + 1, _pos.y, _pos.z });
	BlockType nw = _world.GetBlockId({ _pos.x - 1, _pos.y, _pos.z - 1 });
	BlockType ne = _world.GetBlockId({ _pos.x + 1, _pos.y, _pos.z - 1 });
	BlockType se = _world.GetBlockId({ _pos.x + 1, _pos.y, _pos.z + 1 });
	BlockType sw = _world.GetBlockId({ _pos.x - 1, _pos.y, _pos.z + 1 });

	bool wheatOnXAxis = (west == BLOCK_CROP_WHEAT || east == BLOCK_CROP_WHEAT);
	bool wheatOnZAxis = (north == BLOCK_CROP_WHEAT || south == BLOCK_CROP_WHEAT);
	bool wheatDiagonal = (nw == BLOCK_CROP_WHEAT || ne == BLOCK_CROP_WHEAT || se == BLOCK_CROP_WHEAT ||
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

static bool CanRedstoneComponentStay(WorldManager& _world, Int3 _pos) {
	return _world.IsBlockNormalCube(_pos.Offset(Direction::Value::Down));
}

static bool IsReplaceable(WorldManager& _world, Int3 _pos) {
	BlockType existing = _world.GetBlockId(_pos);
	return existing == BLOCK_AIR || existing == BLOCK_WATER_FLOWING || existing == BLOCK_WATER_STILL ||
	       existing == BLOCK_LAVA_FLOWING || existing == BLOCK_LAVA_STILL || existing == BLOCK_FIRE ||
	       existing == BLOCK_SNOW_LAYER;
};

static bool IsSupported(WorldManager& _world, Int3 _pos, Direction::Value _dir) {
	Int3 support = _pos.WithOffset(Direction::Opposite(_dir));
	return _world.IsBlockNormalCube(support);
};

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

static bool SearchForLog(int _sLength, Int3 _pos, Int3 _cameFrom, WorldManager& _world) {
	auto thisBlock = _world.GetBlockId(_pos);
	if (thisBlock == BLOCK_LOG)
		return true;
	if (_sLength >= 4 || thisBlock != BLOCK_LEAVES)
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

static void TryLavaHarden(WorldManager& _world, Int3 _pos) {
	// Make sure we are lava
	if (_world.GetMaterial(_pos).type != MaterialType::Lava)
		return;

	bool canHarden = false;

	// Check around and above us
	int d[4] = { -1, 1, 0, 0 };
	for (int i = 0; i < 4; i++) {
		int dx = _pos.x + d[i];
		int dz = _pos.z + d[3 - i];
		canHarden = _world.GetMaterial({ dx, _pos.y, dz }).type == MaterialType::Water;
		if (canHarden)
			break;
	}

	// Check above
	if (!canHarden)
		canHarden = _world.GetMaterial(_pos.WithOffset(Direction::Value::Up)).type == MaterialType::Water;

	// We can harden
	if (canHarden) {
		auto myLevel = _world.GetMetadata(_pos);
		if (myLevel == 0) {
			// Obsidian
			_world.SetBlock(_pos, BLOCK_OBSIDIAN);
		} else if (myLevel > 0 && myLevel <= 4) {
			_world.SetBlock(_pos, BLOCK_COBBLESTONE);
		}
	}
}

static bool BlocksFlow(BlockType _block) {
	auto blockMaterial = blockProperties[_block].material;
	if (_block == BLOCK_DOOR_WOOD || _block == BLOCK_DOOR_IRON || _block == BLOCK_SIGN_STANDING ||
	    _block == BLOCK_LADDER || _block == BLOCK_SUGARCANE)
		return true;
	return blockMaterial.isSolid;
}

static bool IsDisplaceable(BlockType _block, MaterialType _fluidMaterialType) {
	auto blockMaterial = blockProperties[_block].material;
	if (blockMaterial.type == _fluidMaterialType)
		return false;
	if (blockMaterial.type == MaterialType::Lava)
		return false;
	return !BlocksFlow(_block);
}

static bool IsOpenForFlow(WorldManager& _world, Int3 _pos, MaterialType _fluidMaterialType) {
	auto block = _world.GetBlockId(_pos);
	auto material = _world.GetMaterial(_pos);
	return !BlocksFlow(block) && (material.type != _fluidMaterialType || _world.GetMetadata(_pos) != 0);
}

static int CalculateFlowCost(WorldManager& _world, Int3 _pos, Int2 _cameFrom, int _depth,
                             MaterialType _fluidMaterialType) {
	int lowest = 1000;
	Int2 opposite = _cameFrom * Int2{ -1, -1 };
	int d[4] = { -1, 1, 0, 0 };
	for (int i = 0; i < 4; i++) {
		int dx = d[i];
		int dz = d[3 - i];
		if (Int2{ dx, dz } == opposite)
			continue;

		Int3 neighborPos = { _pos.x + dx, _pos.y, _pos.z + dz };
		if (IsOpenForFlow(_world, neighborPos, _fluidMaterialType)) {
			Int3 belowNeighbor = neighborPos;
			belowNeighbor.y--;
			if (!BlocksFlow(_world.GetBlockId(belowNeighbor))) {
				return _depth;
			} else if (_depth < 4) {
				lowest = std::min(lowest,
				                  CalculateFlowCost(_world, neighborPos, { dx, dz }, _depth + 1, _fluidMaterialType));
			}
		}
	}
	return lowest;
}

static Vec3 GetFluidFlowVector(WorldManager& _world, Int3 _pos) {
	auto waterMaterial = Material::Water();
	Vec3 flowVector{};
	auto getEffectiveFlowDecay = [&](WorldManager& _lWorld, Int3 _lPos, Material _lMaterial) {
		if (_lWorld.GetMaterial(_lPos) != _lMaterial)
			return -1;
		int meta = _lWorld.GetMetadata(_lPos);
		if (meta >= 8)
			meta = 0;

		return meta;
	};

	int myFlowContribution = getEffectiveFlowDecay(_world, _pos, waterMaterial);

	// Get the contribution of our horizontal neighbors
	int ndx[] = { -1, 1, 0, 0 };
	int ndz[] = { 0, 0, -1, 1 };
	for (int i = 0; i < 4; i++) {
		int dx = _pos.x + ndx[i];
		int dz = _pos.z + ndz[i];
		int neighborFlowContribution = getEffectiveFlowDecay(_world, { dx, _pos.y, dz }, waterMaterial);
		int flowDifference = 0;
		// Our neighbor block didn't have the same material
		if (neighborFlowContribution < 0) {
			if (!_world.GetMaterial({ dx, _pos.y, dz }).isSolid) {
				// Check the block below us to see if its water, if it is, STRONGLY pull down
				int belowFlowContribution = getEffectiveFlowDecay(_world, { dx, _pos.y - 1, dz }, waterMaterial);
				if (belowFlowContribution >= 0) {
					flowDifference = belowFlowContribution - (myFlowContribution - 8);
					flowVector.x += double((dx - _pos.x) * flowDifference);
					flowVector.z += double((dz - _pos.z) * flowDifference);
				}
			}
		} else {
			flowDifference = neighborFlowContribution - myFlowContribution;
			flowVector.x += double((dx - _pos.x) * flowDifference);
			flowVector.z += double((dz - _pos.z) * flowDifference);
		}
	}

	auto isFluidWall = [&](Int3 _checkPos) {
		Material neighborMaterial = _world.GetMaterial(_checkPos);
		if (neighborMaterial == waterMaterial)
			return false;
		if (neighborMaterial == Material::Ice())
			return false;
		return neighborMaterial.isSolid;
	};

	// If we're a falling fluid segment, check whether we're clinging to a wall
	if (_world.GetMetadata(_pos) >= 8) {
		bool nearWall = false;

		if (!nearWall && isFluidWall(_pos.WithOffset(Direction::Value::North)))
			nearWall = true;
		if (!nearWall && isFluidWall(_pos.WithOffset(Direction::Value::South)))
			nearWall = true;
		if (!nearWall && isFluidWall(_pos.WithOffset(Direction::Value::West)))
			nearWall = true;
		if (!nearWall && isFluidWall(_pos.WithOffset(Direction::Value::East)))
			nearWall = true;
		if (!nearWall && isFluidWall(_pos.WithOffset(Direction::Value::Up).Offset(Direction::Value::North)))
			nearWall = true;
		if (!nearWall && isFluidWall(_pos.WithOffset(Direction::Value::Up).Offset(Direction::Value::South)))
			nearWall = true;
		if (!nearWall && isFluidWall(_pos.WithOffset(Direction::Value::Up).Offset(Direction::Value::West)))
			nearWall = true;
		if (!nearWall && isFluidWall(_pos.WithOffset(Direction::Value::Up).Offset(Direction::Value::East)))
			nearWall = true;

		if (nearWall) {
			double lenSq = flowVector.x * flowVector.x + flowVector.y * flowVector.y + flowVector.z * flowVector.z;
			if (lenSq > 0.0) {
				double invLen = 1.0 / std::sqrt(lenSq);
				flowVector.x *= invLen;
				flowVector.y *= invLen;
				flowVector.z *= invLen;
			}
			flowVector.y += -6.0;
		}
	}

	double lenSq = flowVector.x * flowVector.x + flowVector.y * flowVector.y + flowVector.z * flowVector.z;
	if (lenSq > 0.0) {
		double invLen = 1.0 / std::sqrt(lenSq);
		flowVector.x *= invLen;
		flowVector.y *= invLen;
		flowVector.z *= invLen;
	}

	return flowVector;
}

static int GetDirectionFromYaw(float _yaw, int _directionCount) {
	return MathHelper::FloorDouble((_yaw * _directionCount / 360.0f) + 0.5f) & 3;
}

void GenericExplode(WorldManager& _world, Int3 _pos) {
	BreakAndDropBlockWithChance(_world, _pos, 0.3f);
}

void GenericBreak(WorldManager& _world, Int3 _pos, Entity& _destroyer) {
	BreakAndDropBlock(_world, _pos);
}

bool GenericPlace(WorldManager& _world, Int3 _pos, [[maybe_unused]] Entity& _placer, Direction::Value _face,
                  BlockType _blockId, uint8_t _meta) {
	if (!_world.InBounds(_pos.y))
		return false;

	BlockType existing = _world.GetBlockId(_pos);
	Int3 sourceBlock = _pos.WithOffset(Direction::Opposite(_face));

	// Check if we can replace snow
	Int3 targetPos = _pos;
	if (_world.GetBlockId(sourceBlock) == BLOCK_SNOW_LAYER) {
		targetPos = sourceBlock;
	} else {
		// Check if this block is replaceable
		// Should match vanilla?
		bool replaceable = existing == BLOCK_AIR || existing == BLOCK_WATER_FLOWING || existing == BLOCK_WATER_STILL ||
		                   existing == BLOCK_LAVA_FLOWING || existing == BLOCK_LAVA_STILL || existing == BLOCK_FIRE ||
		                   existing == BLOCK_SNOW_LAYER;
		if (!replaceable)
			return false;
	}

	// Check to see if any entities overlap our block's collider
	if (Blocks::blockProperties[_blockId].isCollidable) {
		auto blockCollider = Blocks::blockBehaviors[_blockId].getCollider(_meta).Offset(
		    targetPos.x, targetPos.y,
		    targetPos.z); // Block colliders are at the origin so shift to world space
		auto entitiesInBlock = _world.entityManager.GetEntitiesWithinAabb(
		    { double(targetPos.x), double(targetPos.y), double(targetPos.z), double(targetPos.x) + 1.0,
		      double(targetPos.y) + 1.0, double(targetPos.z) + 1.0 });
		for (auto& entity : entitiesInBlock) {
			if (blockCollider.Intersects(entity->collider) && entity->preventEntitySpawning) {
				return false;
			}
		}
	}

	_world.SetBlock(targetPos, _blockId, _meta);
	return true;
}

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

void RegisterBlockBehaviors() {
	// Liquids/zero-size AABBs
	blockBehaviors[BlockType::BLOCK_WATER_FLOWING] = {
		.getSelectionBox = LiquidAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_WATER_STILL] = {
		.getSelectionBox = LiquidAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_LAVA_FLOWING] = {
		.getSelectionBox = LiquidAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_LAVA_STILL] = {
		.getSelectionBox = LiquidAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_COBWEB] = {
		.getCollider = EmptyCollider,
	};

	// Rails
	blockBehaviors[BlockType::BLOCK_RAIL] = {
		.getRayBounds = RailAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_RAIL_POWERED] = {
		.getRayBounds = RailAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_RAIL_DETECTOR] = {
		.getRayBounds = RailAabb,
		.getCollider = EmptyCollider,
	};

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

	blockBehaviors[BlockType::BLOCK_CACTUS] = {
		.getSelectionBox = CactusAabb,
		.getRayBounds = CactusAabb,
		.getCollider = CactusCollider,
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
		.onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face,
		                    BlockType _blockId, uint8_t _meta) -> bool {
		    // Doors can only be placed against the sides of blocks
		    if (_face == Direction::Value::Up || _face == Direction::Value::Down)
			    return false;

		    if (!IsReplaceable(_world, _pos))
			    return false;

		    _world.SetBlock(_pos, _blockId, GetMetaFromDirection(BLOCK_TRAPDOOR, Direction::Opposite(_face)));
		    return true;
		}
	};

	blockBehaviors[BlockType::BLOCK_BED] = {
		.getSelectionBox = BedAabb,
		.getRayBounds = BedAabb,
		.getCollider = BedCollider,
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
		.onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta, Java::Random& _random) -> void {
		    // Check to make sure we can till exist here
		    if (!IsSupported(_world, _pos, GetDirectionFromMeta(BLOCK_BUTTON_STONE, _meta)))
			    BreakAndDropBlock(_world, _pos);
		    // Unpress again
		    if (_meta & 0b1000) {
			    _world.SetMeta(_pos, _meta & 0b111);
			    NotifyAttachedSupportBlock(_world, _pos, BLOCK_BUTTON_STONE, _meta);
		    }
		},
		.onNeighborBlockChange = [](WorldManager& _world, Int3 _pos, BlockType _blockId) -> void {
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
		.onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face,
		                    BlockType _blockId, uint8_t _meta) -> bool {
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

	blockBehaviors[BlockType::BLOCK_PISTON_HEAD] = {
		.getSelectionBox = PistonHeadAabb,
		.getRayBounds = PistonHeadAabb,
		.getCollider = PistonHeadCollider,
	};

	blockBehaviors[BLOCK_SOULSAND] = {
		.getCollider = SoulSandCollider,
	};

	// Specific behavioral overrides
	blockBehaviors[BLOCK_WATER_FLOWING].velocityToAddToEntity = [](WorldManager& _world, Int3 _pos,
	                                                               Vec3& _pushVector) -> void {
		Vec3 flowVector = GetFluidFlowVector(_world, _pos);
		_pushVector = _pushVector + flowVector;
	};
	blockBehaviors[BLOCK_WATER_STILL].velocityToAddToEntity = [](WorldManager& _world, Int3 _pos,
	                                                             Vec3& _pushVector) -> void {
		Vec3 flowVector = GetFluidFlowVector(_world, _pos);
		_pushVector = _pushVector + flowVector;
	};
	blockBehaviors[BLOCK_CACTUS].onEntityCollidedWithBlock = [](WorldManager& _world, Int3 _pos,
	                                                            Entity& _entity) -> void {
		_entity.AttackEntityFrom(nullptr, 1);
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
	blockBehaviors[BLOCK_CROP_WHEAT].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                            BlockType _blockId) -> void {
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
	                                                           BlockType _blockId) -> void {
		// Check to see if our placement is still valid
		if (!CanSugarcaneSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
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

	auto onPistonPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face,
	                        BlockType _blockId, uint8_t _meta) -> bool {
		return GenericPlace(_world, _pos, _placer, _face, _blockId, GetMetaFromDirection(BLOCK_PISTON, _face));
	};

	// Farmland
	blockBehaviors[BLOCK_FARMLAND].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                          BlockType _blockId) -> void {
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
		auto WaterNearby = [](WorldManager& _world, Int3 _pos) -> bool {
			for (int dx = -4; dx <= 4; dx++) {
				for (int dz = -4; dz <= 4; dz++) {
					for (int dy = 0; dy <= 1; dy++) {
						Int3 checkPos = { _pos.x + dx, _pos.y + dy, _pos.z + dz };
						if (_world.GetMaterial(checkPos) == Material::Water())
							return true;
					}
				}
			}
			return false;
		};

		// There is water so max hydration
		if (WaterNearby(_world, _pos)) {
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
	                                                        BlockType _blockId) -> void {
		blockBehaviors[BLOCK_CACTUS].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_MUSHROOM_BROWN].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                                BlockType _blockId) -> void {
		blockBehaviors[BLOCK_MUSHROOM_BROWN].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_MUSHROOM_RED].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                              BlockType _blockId) -> void {
		blockBehaviors[BLOCK_MUSHROOM_RED].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_DANDELION].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                           BlockType _blockId) -> void {
		blockBehaviors[BLOCK_DANDELION].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_ROSE].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos, BlockType _blockId) -> void {
		blockBehaviors[BLOCK_ROSE].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_TALLGRASS].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                           BlockType _blockId) -> void {
		blockBehaviors[BLOCK_TALLGRASS].onTick(_world, _pos, _world.GetMetadata(_pos), _world.rand);
	};
	blockBehaviors[BLOCK_SAPLING].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                         BlockType _blockId) -> void {
		if (!CanGenericPlantSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_DANDELION].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                            Java::Random& _random) -> void {
		if (!CanGenericPlantSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_ROSE].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                       Java::Random& _random) -> void {
		if (!CanGenericPlantSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_TALLGRASS].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                            Java::Random& _random) -> void {
		if (!CanGenericPlantSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_MUSHROOM_BROWN].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                                 Java::Random& _random) -> void {
		if (!CanMushroomSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_MUSHROOM_RED].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                               Java::Random& _random) -> void {
		if (!CanMushroomSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};
	blockBehaviors[BLOCK_CACTUS].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                         Java::Random& _random) -> void {
		if (!CanCactusSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};

	blockBehaviors[BLOCK_PISTON].onBlockPlaced = onPistonPlace;
	blockBehaviors[BLOCK_PISTON_STICKY].onBlockPlaced = onPistonPlace;

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

	blockBehaviors[BLOCK_REDSTONE_TORCH_ON].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                                           Direction::Value _face, BlockType _blockId,
	                                                           uint8_t _meta) -> bool {
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
	                                                                   BlockType _blockId) -> void {
		auto dir = GetDirectionFromMeta(BLOCK_REDSTONE_TORCH_ON, _world.GetMetadata(_pos));
		if (!CanTorchAttachTo(_world, _pos, dir)) {
			BreakAndDropBlock(_world, _pos);
			return;
		}

		// Torches take two ticks to update
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_REDSTONE_TORCH_ON, 2);
	};

	blockBehaviors[BLOCK_REDSTONE_TORCH_ON].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                                    Java::Random& _random) -> void {
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
	                                                                    BlockType _blockId) -> void {
		const auto dir = GetDirectionFromMeta(BLOCK_REDSTONE_TORCH_OFF, _world.GetMetadata(_pos));
		if (!CanTorchAttachTo(_world, _pos, dir)) {
			BreakAndDropBlock(_world, _pos);
			return;
		}

		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_REDSTONE_TORCH_OFF, 2);
	};

	blockBehaviors[BLOCK_REDSTONE_TORCH_OFF].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                                     Java::Random& _random) -> void {
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
	                                                               uint8_t _meta) -> bool {
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
	                                                                       BlockType _blockId) -> void {
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
	                                                        Java::Random& _random) -> void {
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
	                                               Direction::Value _face, BlockType _blockId, uint8_t _meta) -> bool {
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

	blockBehaviors[BLOCK_LEVER].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos, BlockType _blockId) -> void {
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
	                                               Direction::Value _face, BlockType _blockId, uint8_t _meta) -> bool {
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

	blockBehaviors[BLOCK_TORCH].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos, BlockType _blockId) -> void {
		const auto dir = GetDirectionFromMeta(BLOCK_TORCH, _world.GetMetadata(_pos));
		if (!CanTorchAttachTo(_world, _pos, dir))
			BreakAndDropBlock(_world, _pos);
	};

	auto onDoorPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, Direction::Value _face, BlockType _blockId,
	                      uint8_t _meta) -> bool {
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

	// for when the block is interacted with!
	blockBehaviors[BLOCK_REDSTONE_REPEATER_OFF].onBlockActivated = [](WorldManager& _world, Int3 _pos,
	                                                                  PlayerSession* _triggeringSession) -> bool {
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
	blockBehaviors[BLOCK_DOOR_WOOD].onBlockActivated = [](WorldManager& _world, Int3 _pos,
	                                                      PlayerSession* _triggeringSession) -> bool {
		ToggleDoor(_world, _pos, _triggeringSession);
		return false;
	};
	blockBehaviors[BLOCK_DOOR_WOOD].onBlockClicked = ToggleDoor;
	blockBehaviors[BLOCK_DOOR_WOOD].onBlockDestroyedByPlayer = [](WorldManager& _world, Int3 _pos, Entity& _destroyer) {
		BreakDoor(_world, _pos, BLOCK_DOOR_WOOD);
	};
	blockBehaviors[BLOCK_DOOR_IRON].onBlockDestroyedByPlayer = [](WorldManager& _world, Int3 _pos, Entity& _destroyer) {
		BreakDoor(_world, _pos, BLOCK_DOOR_IRON);
	};

	blockBehaviors[BLOCK_BED].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                             Direction::Value _face, BlockType _blockId, uint8_t _meta) -> bool {
		// Beds can only be placed by clicking the top face of a block
		if (_face != Direction::Value::Up)
			return false;

		// _pos is already the target cell
		const Int3 placePos = _pos;
		if (!_world.InBounds(placePos.y))
			return false;

		const auto dir = Direction::FromAngle(_placer.rotationYaw);
		const Int3 headPos = placePos.WithOffset(dir);

		// Is this placement valid?
		if (!_world.IsBlockNormalCube(placePos.WithOffset(Direction::Value::Down)) ||
		    !_world.IsBlockNormalCube(headPos.WithOffset(Direction::Value::Down)))
			return false;
		if (!IsReplaceable(_world, placePos) || !IsReplaceable(_world, headPos))
			return false;

		uint8_t meta = GetMetaFromDirection(BLOCK_BED, dir);
		_world.SetBlock(placePos, _blockId, meta);
		_world.SetBlock(headPos, _blockId, uint8_t(meta | 0b1000));

		return true;
	};
	//blockBehaviors[BLOCK_BED].onBlockClicked = ToggleDoor;
	blockBehaviors[BLOCK_BED].onBlockDestroyedByPlayer = [](WorldManager& _world, Int3 _pos, Entity& _destroyer) {
		auto meta = _world.GetMetadata(_pos);
		auto dir = GetDirectionFromMeta(BLOCK_BED, meta);
		if (meta & 0b1000) {
			// We are the head of the bed
			// Offset one down
			_pos.Offset(Direction::Opposite(dir));
			if (_world.GetBlockId(_pos) != BLOCK_BED)
				// The foot is not a bed block!
				return;
		}
		// Since we're now guaranteed to be pointing at the foot of the bed,
		// we can continue like this
		Int3 headPos = _pos.WithOffset(dir);
		if (_world.GetBlockId(headPos) == BLOCK_BED && (_world.GetMetadata(headPos) & 0b1000)) {
			_world.SetBlock(headPos, BLOCK_AIR);
		}
		BreakAndDropBlock(_world, _pos);
	};

	// Grass spread / decay
	blockBehaviors[BLOCK_GRASS].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
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
	                                         Java::Random& _random) -> void {
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

	// Falling blocks!
	blockBehaviors[BLOCK_GRAVEL].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                        BlockType _blockId) -> void {
		// Schedule a check to see if we can fall
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_GRAVEL, 3);
	};
	blockBehaviors[BLOCK_GRAVEL].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_GRAVEL, 3);
	};
	blockBehaviors[BLOCK_GRAVEL].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                         Java::Random& _random) -> void {
		const Int3 below = _pos.WithOffset(Direction::Value::Down);

		if (!Blocks::CanFallAt(_world, below) || _pos.y < 0)
			return;

		constexpr int32_t CHECK_RADIUS = 32; // Blocks
		bool areaLoaded = _world.AABBinValidChunks({ double(_pos.x - CHECK_RADIUS), double(_pos.y),
		                                             double(_pos.z - CHECK_RADIUS), double(_pos.x + CHECK_RADIUS),
		                                             double(_pos.y), double(_pos.z + CHECK_RADIUS) });

		if (areaLoaded) {
			Vec3 spawnPos = { _pos.x + 0.5, _pos.y + 0.5, _pos.z + 0.5 };
			auto entity = std::make_shared<FallingBlockEntity>(spawnPos, BLOCK_GRAVEL);
			_world.entityManager.AddEntity(std::move(entity));
			_world.SetBlock(_pos, BLOCK_AIR, 0);
		} else {
			_world.SetBlock(_pos, BLOCK_AIR, 0);

			Int3 landing = _pos;
			while (Blocks::CanFallAt(_world, landing.WithOffset(Direction::Value::Down)) && landing.y > 0)
				landing.y--;

			if (landing.y > 0)
				_world.SetBlock(landing, BLOCK_GRAVEL, 0);
		}
	};
	blockBehaviors[BLOCK_SAND].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos, BlockType _blockId) -> void {
		// Schedule a check to see if we can fall
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_SAND, 3);
	};
	blockBehaviors[BLOCK_SAND].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_SAND, 3);
	};
	blockBehaviors[BLOCK_SAND].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                       Java::Random& _random) -> void {
		Int3 below = { _pos.x, _pos.y - 1, _pos.z };

		if (!Blocks::CanFallAt(_world, below) || _pos.y < 0)
			return;

		constexpr int32_t CHECK_RADIUS = 32; // Blocks
		bool areaLoaded = _world.AABBinValidChunks({ double(_pos.x - CHECK_RADIUS), double(_pos.y),
		                                             double(_pos.z - CHECK_RADIUS), double(_pos.x + CHECK_RADIUS),
		                                             double(_pos.y), double(_pos.z + CHECK_RADIUS) });

		if (areaLoaded) {
			_world.SetBlock(_pos, BLOCK_AIR, 0);
			Vec3 spawnPos = { _pos.x + 0.5, _pos.y + 0.5, _pos.z + 0.5 };
			auto entity = std::make_shared<FallingBlockEntity>(spawnPos, BLOCK_SAND);
			_world.entityManager.AddEntity(std::move(entity));
		} else {
			_world.SetBlock(_pos, BLOCK_AIR, 0);

			Int3 landing = _pos;
			while (Blocks::CanFallAt(_world, { landing.x, landing.y - 1, landing.z }) && landing.y > 0)
				landing.y--;

			if (landing.y > 0)
				_world.SetBlock(landing, BLOCK_SAND, 0);
		}
	};

	blockBehaviors[BLOCK_CHEST].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		auto chest = std::make_shared<TileEntityChest>(_pos);
		_world.CreateTileEntity(std::move(chest));
	};
	blockBehaviors[BLOCK_CHEST].onBlockRemoval = [](WorldManager& _world, Int3 _pos) -> void {
		auto* te = _world.GetTileEntityAs<TileEntityChest>(_pos);
		if (!te)
			return;

		_world.DropInventory(te->inventory, _pos);
	};

	blockBehaviors[BLOCK_FURNACE].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		auto furnace = std::make_shared<TileEntityFurnace>(_pos);
		_world.CreateTileEntity(std::move(furnace));
	};

	auto dropFurnaceInventory = [](WorldManager& _world, Int3 _pos) -> void {
		auto* te = _world.GetTileEntityAs<TileEntityFurnace>(_pos);
		if (!te)
			return;

		_world.DropInventory(te->inventory, _pos);
	};

	blockBehaviors[BLOCK_FURNACE].onBlockRemoval = dropFurnaceInventory;
	blockBehaviors[BLOCK_FURNACE_LIT].onBlockRemoval = dropFurnaceInventory;

	// TODO: Add another portal creation function matching with b1.7.3's limitations, that is toggleable via a config entry,
	// for a more authentic experience
	blockBehaviors[BLOCK_FIRE].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		bool isXAligned = (_world.GetBlockId(_pos + Int3{ 1, -1, 0 }) == BLOCK_OBSIDIAN ||
		                   _world.GetBlockId(_pos + Int3{ -1, -1, 0 }) == BLOCK_OBSIDIAN);

		bool isZAligned = (_world.GetBlockId(_pos + Int3{ 0, -1, 1 }) == BLOCK_OBSIDIAN ||
		                   _world.GetBlockId(_pos + Int3{ 0, -1, -1 }) == BLOCK_OBSIDIAN);

		if (!isXAligned && !isZAligned)
			return;

		std::array<Int3, 4> neighbors =
		    isXAligned ? std::array<Int3, 4>{ { { 0, 1, 0 }, { 0, -1, 0 }, { 1, 0, 0 }, { -1, 0, 0 } } }
		               : std::array<Int3, 4>{ { { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } } };

		std::vector<Int3> queue = { _pos };
		size_t head = 0;
		constexpr size_t MAX_PORTAL_BLOCKS = 21 * 21;

		while (head < queue.size()) {
			Int3 curr = queue[head++];

			if (queue.size() > MAX_PORTAL_BLOCKS)
				return;

			for (const auto& offset : neighbors) {
				Int3 next = curr + offset;
				BlockType id = _world.GetBlockId(next);

				if (id == BLOCK_OBSIDIAN)
					continue;

				if (id != BLOCK_AIR && id != BLOCK_FIRE && id != BLOCK_NETHER_PORTAL)
					return;

				if (std::find(queue.begin(), queue.end(), next) == queue.end()) {
					queue.push_back(next);
				}
			}
		}

		if (queue.empty())
			return;

		int minX = queue[0].x, maxX = queue[0].x;
		int minY = queue[0].y, maxY = queue[0].y;
		int minZ = queue[0].z, maxZ = queue[0].z;

		for (const auto& block : queue) {
			if (block.x < minX)
				minX = block.x;
			if (block.x > maxX)
				maxX = block.x;
			if (block.y < minY)
				minY = block.y;
			if (block.y > maxY)
				maxY = block.y;
			if (block.z < minZ)
				minZ = block.z;
			if (block.z > maxZ)
				maxZ = block.z;
		}

		int width = isXAligned ? (maxX - minX + 1) : (maxZ - minZ + 1);
		int height = maxY - minY + 1;

		constexpr int MIN_WIDTH = 2;
		constexpr int MIN_HEIGHT = 3;

		if (width < MIN_WIDTH || height < MIN_HEIGHT)
			return;

		for (const auto& innerPos : queue)
			_world.SetBlock(innerPos, BLOCK_NETHER_PORTAL);
	};

	// Lava physics
	blockBehaviors[BLOCK_LAVA_FLOWING].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		// Schedule ourselves for an update
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_LAVA_FLOWING,
		                                        _world.GetDimension() == Dimension::Nether ? 10 : 30);
		TryLavaHarden(_world, _pos);
	};
	blockBehaviors[BLOCK_LAVA_FLOWING].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                              BlockType _blockId) -> void {
		// Stack overflow if this was regular set block!
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_LAVA_FLOWING,
		                                        _world.GetDimension() == Dimension::Nether ? 10 : 30);
		TryLavaHarden(_world, _pos);
	};
	blockBehaviors[BLOCK_LAVA_STILL].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                            BlockType _blockId) -> void {
		// Stack overflow if this was regular set block!
		_world.SetBlockRaw(_pos, BLOCK_LAVA_FLOWING, _world.GetMetadata(_pos));
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_LAVA_FLOWING,
		                                        _world.GetDimension() == Dimension::Nether ? 10 : 30);
		TryLavaHarden(_world, _pos);
	};
	blockBehaviors[BLOCK_LAVA_STILL].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		TryLavaHarden(_world, _pos);
	};
	blockBehaviors[BLOCK_LAVA_FLOWING].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                               Java::Random& _random) -> void {
		auto level = _meta % 8;
		bool isFalling = _meta >= 8;
		bool isSource = _meta == 0;
		auto candidateLevel = -1;
		Int3 belowPos = { _pos.x, _pos.y - 1, _pos.z };

		int stepDecay = _world.GetDimension() == Dimension::Nether ? 1 : 2;

		// Are we a source block?
		if (!isSource) {
			// We aren't a source block so we need to update our level
			int8_t adjacentSourceCount = 0;
			int lowestNeighborLevel = 999;
			int d[4] = { -1, 1, 0, 0 };
			for (int i = 0; i < 4; i++) {
				auto dx = _pos.x + d[i];
				auto dz = _pos.z + d[3 - i];
				Int3 neighborPos = { dx, _pos.y, dz };
				if (_world.GetMaterial(neighborPos).type == MaterialType::Lava) {
					// Falling water (>= 8) is treated as level 0
					auto neighborLevel = _world.GetMetadata({ dx, _pos.y, dz });
					auto effectiveLevel = neighborLevel >= 8 ? 0 : neighborLevel;
					// Yes !neighborLevel would work here but this is more explicit
					if (neighborLevel == 0)
						adjacentSourceCount++;
					if (effectiveLevel < lowestNeighborLevel)
						lowestNeighborLevel = effectiveLevel;
				}

				// If lowestNeighborLevel is 999, then no neighbors were liquids
				// If we are level 8 or higher we are fully exhausted, so remove ourselves
				candidateLevel = lowestNeighborLevel + stepDecay;
				bool invalid = lowestNeighborLevel == 999 || candidateLevel >= 8;
				if (invalid)
					candidateLevel = -1;
			}

			// Check for vertical feed
			if (_world.GetMaterial({ _pos.x, _pos.y + 1, _pos.z }).type == MaterialType::Lava) {
				// If our above level is already falling, then just copy it
				// If it isn't, convert ourselves to falling by adding 8
				auto aboveLevel = _world.GetMetadata({ _pos.x, _pos.y + 1, _pos.z });
				candidateLevel = (aboveLevel >= 8) ? aboveLevel : aboveLevel + 8;
			}

			// Lava has some flow hesitation behavior
			bool heldByHesitation = false;
			if (_meta < 8 && candidateLevel < 8 && candidateLevel > _meta) {
				if (_world.rand.NextInt(4) != 0) {
					candidateLevel = _meta;
					heldByHesitation = true;
				}
			}

			if (candidateLevel == -1) {
				_world.SetBlock(_pos, BLOCK_AIR);
				return;
			} else if (candidateLevel != _meta) {
				_world.SetMeta(_pos, candidateLevel);
				level = candidateLevel;
				_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_LAVA_FLOWING,
				                                        _world.GetDimension() == Dimension::Nether ? 10 : 30);
			} else if (heldByHesitation) {
				_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_LAVA_FLOWING,
				                                        _world.GetDimension() == Dimension::Nether ? 10 : 30);
			} else {
				_world.SetBlockRaw(_pos, BLOCK_LAVA_STILL, _meta);
			}
		} else {
			// We are a source so convert ourselves
			_world.SetBlockRaw(_pos, BLOCK_LAVA_STILL, level);
		}

		auto belowBlock = _world.GetBlockId(belowPos);
		if (IsDisplaceable(belowBlock, MaterialType::Lava)) {
			_world.SetBlock(belowPos, BLOCK_LAVA_FLOWING, (level >= 8) ? level : level + 8);
			return;
		}

		// Only spread sideways if we're a source, or what's below us actually blocks flow.
		if (level != 0 && !BlocksFlow(belowBlock)) {
			return;
		}

		// We only reach the horizontal spread if we are a source
		// or we couldn't fall down
		int directionalCosts[4] = { 1000, 1000, 1000, 1000 };
		int minDirectionalCost = 1000;
		int directions[4] = { -1, 1, 0, 0 };
		for (int i = 0; i < 4; i++) {
			// Check if we can find a close hole to flow towards
			auto dx = _pos.x + directions[i];
			auto dz = _pos.z + directions[3 - i];

			// We can flow in this direction
			Int3 neighborPos = { dx, _pos.y, dz };
			if (IsOpenForFlow(_world, neighborPos, MaterialType::Lava)) {
				Int3 belowNeighborPos = { dx, _pos.y - 1, dz };
				if (!BlocksFlow(_world.GetBlockId(belowNeighborPos))) {
					directionalCosts[i] = 0; // Immediate drop off
				} else {
					int initialStep = 1;
					directionalCosts[i] = CalculateFlowCost(_world, neighborPos, { dx, dz }, initialStep,
					                                        MaterialType::Lava);
				}
				if (directionalCosts[i] < minDirectionalCost)
					minDirectionalCost = directionalCosts[i];
			}
		}

		// Spread outwards
		int outLevel = isFalling ? 1 : level + stepDecay;
		if (outLevel >= 8)
			return;

		for (int i = 0; i < 4; i++) {
			if (directionalCosts[i] > minDirectionalCost)
				continue;
			auto dx = _pos.x + directions[i];
			auto dz = _pos.z + directions[3 - i];
			Int3 newPos = { dx, _pos.y, dz };
			if (IsDisplaceable(_world.GetBlockId(newPos), MaterialType::Lava)) {
				// Lava never drops what it displaces
				_world.SetBlock(newPos, BLOCK_LAVA_FLOWING, outLevel);
			}
		}
	};

	// FLUID PHYSICS (water)
	blockBehaviors[BLOCK_WATER_FLOWING].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		// Schedule ourselves for an update
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_WATER_FLOWING, 5);
	};
	blockBehaviors[BLOCK_WATER_FLOWING].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                               BlockType _blockId) -> void {
		// Stack overflow if this was regular set block!
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_WATER_FLOWING, 5);
	};
	blockBehaviors[BLOCK_WATER_STILL].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                             BlockType _blockId) -> void {
		// Stack overflow if this was regular set block!
		_world.SetBlockRaw(_pos, BLOCK_WATER_FLOWING, _world.GetMetadata(_pos));
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_WATER_FLOWING, 5);
	};
	blockBehaviors[BLOCK_WATER_FLOWING].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                                Java::Random& _random) -> void {
		auto level = _meta % 8;
		bool isFalling = _meta >= 8;
		bool isSource = _meta == 0;
		auto candidateLevel = -1;
		Int3 belowPos = { _pos.x, _pos.y - 1, _pos.z };

		int stepDecay = 1;

		// Are we a source block?
		if (!isSource) {
			// We aren't a source block so we need to update our level
			int8_t adjacentSourceCount = 0;
			int lowestNeighborLevel = 999;
			int d[4] = { -1, 1, 0, 0 };
			for (int i = 0; i < 4; i++) {
				auto dx = _pos.x + d[i];
				auto dz = _pos.z + d[3 - i];
				Int3 neighborPos = { dx, _pos.y, dz };
				if (_world.GetMaterial(neighborPos).type == MaterialType::Water) {
					// Falling water (>= 8) is treated as level 0
					auto neighborLevel = _world.GetMetadata({ dx, _pos.y, dz });
					auto effectiveLevel = neighborLevel >= 8 ? 0 : neighborLevel;
					// Yes !neighborLevel would work here but this is more explicit
					if (neighborLevel == 0)
						adjacentSourceCount++;
					if (effectiveLevel < lowestNeighborLevel)
						lowestNeighborLevel = effectiveLevel;
				}

				// If lowestNeighborLevel is 999, then no neighbors were liquids
				// If we are level 8 or higher we are fully exhausted, so remove ourselves
				candidateLevel = lowestNeighborLevel + stepDecay;
				bool invalid = lowestNeighborLevel == 999 || candidateLevel >= 8;
				if (invalid)
					candidateLevel = -1;
			}

			// Check for vertical feed
			if (_world.GetMaterial({ _pos.x, _pos.y + 1, _pos.z }).type == MaterialType::Water) {
				// If our above level is already falling, then just copy it
				// If it isn't, convert ourselves to falling by adding 8
				auto aboveLevel = _world.GetMetadata({ _pos.x, _pos.y + 1, _pos.z });
				candidateLevel = (aboveLevel >= 8) ? aboveLevel : aboveLevel + 8;
			}

			// Source regeneration
			// Only turn into a source if the block below us is also water or isSolid is true
			auto belowMaterial = _world.GetMaterial(belowPos);
			bool belowValid = belowMaterial.type == MaterialType::Water || belowMaterial.isSolid;
			if (adjacentSourceCount >= 2 && belowValid)
				candidateLevel = 0;

			// Check if we are valid and update our level
			if (candidateLevel == -1) {
				_world.SetBlock(_pos, BLOCK_AIR);
				return;
			} else if (candidateLevel != _meta) {
				_world.SetMeta(_pos, candidateLevel);
				level = candidateLevel;
				_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_WATER_FLOWING, 5);
			} else {
				_world.SetBlockRaw(_pos, BLOCK_WATER_STILL, _meta);
			}
		} else {
			// We are a source so convert ourselves
			_world.SetBlockRaw(_pos, BLOCK_WATER_STILL, level);
		}

		auto belowBlock = _world.GetBlockId(belowPos);
		if (IsDisplaceable(belowBlock, MaterialType::Water)) {
			_world.SetBlock(belowPos, BLOCK_WATER_FLOWING, (level >= 8) ? level : level + 8);
			return;
		}

		// Only spread sideways if we're a source, or what's below us actually blocks flow.
		if (level != 0 && !BlocksFlow(belowBlock)) {
			return;
		}

		// We only reach the horizontal spread if we are a source
		// or we couldn't fall down
		int directionalCosts[4] = { 1000, 1000, 1000, 1000 };
		int minDirectionalCost = 1000;
		int directions[4] = { -1, 1, 0, 0 };
		for (int i = 0; i < 4; i++) {
			// Check if we can find a close hole to flow towards
			auto dx = _pos.x + directions[i];
			auto dz = _pos.z + directions[3 - i];

			// We can flow in this direction
			Int3 neighborPos = { dx, _pos.y, dz };
			if (IsOpenForFlow(_world, neighborPos, MaterialType::Water)) {
				Int3 belowNeighborPos = { dx, _pos.y - 1, dz };
				auto below = _world.GetBlockId(belowNeighborPos);
				if (!BlocksFlow(below)) {
					directionalCosts[i] = 0; // Immediate drop off
				} else {
					int initialStep = 1;
					directionalCosts[i] = CalculateFlowCost(_world, neighborPos, { dx, dz }, initialStep,
					                                        MaterialType::Water);
				}
				if (directionalCosts[i] < minDirectionalCost)
					minDirectionalCost = directionalCosts[i];
			}
		}

		// Spread outwards
		int outLevel = isFalling ? 1 : level + stepDecay;
		if (outLevel >= 8)
			return;

		for (int i = 0; i < 4; i++) {
			if (directionalCosts[i] > minDirectionalCost)
				continue;
			auto dx = _pos.x + directions[i];
			auto dz = _pos.z + directions[3 - i];
			Int3 newPos = { dx, _pos.y, dz };
			if (IsDisplaceable(_world.GetBlockId(newPos), MaterialType::Water)) {
				BreakAndDropBlock(_world, newPos);
				_world.SetBlock(newPos, BLOCK_WATER_FLOWING, outLevel);
			}
		}
	};

	// --------------- block drops, only exceptions are included (something that doesn't drop itself) ---------------
	blockBehaviors[BLOCK_STONE].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_COBBLESTONE;
	};
	blockBehaviors[BLOCK_GRASS].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_DIRT;
	};
	blockBehaviors[BLOCK_FARMLAND].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_DIRT;
	};
	blockBehaviors[BLOCK_ORE_COAL].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::COAL;
	};
	blockBehaviors[BLOCK_ORE_DIAMOND].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::DIAMOND;
	};
	blockBehaviors[BLOCK_REDSTONE].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE;
	};
	blockBehaviors[BLOCK_SUGARCANE].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::SUGARCANE;
	};
	blockBehaviors[BLOCK_COBWEB].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::STRING;
	};
	blockBehaviors[BLOCK_DEADBUSH].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::INVALID;
	};
	blockBehaviors[BLOCK_STAIRS_WOOD].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_PLANKS;
	};
	blockBehaviors[BLOCK_STAIRS_COBBLESTONE].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_COBBLESTONE;
	};

	blockBehaviors[BLOCK_SIGN_STANDING].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::SIGN;
	};
	blockBehaviors[BLOCK_SIGN_WALL].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::SIGN;
	};
	blockBehaviors[BLOCK_FURNACE_LIT].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_FURNACE;
	};
	blockBehaviors[BLOCK_REDSTONE_REPEATER_OFF].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE_REPEATER;
	};
	blockBehaviors[BLOCK_REDSTONE_REPEATER_ON].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE_REPEATER;
	};
	blockBehaviors[BLOCK_REDSTONE_TORCH_OFF].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_REDSTONE_TORCH_ON;
	};

	// --------------- drop themselves but pass their metadata onto the item ---------------
	blockBehaviors[BLOCK_WOOL].damageDropped = [](uint8_t _meta) -> ItemDamage {
		return _meta;
	};
	blockBehaviors[BLOCK_LOG].damageDropped = [](uint8_t _meta) -> ItemDamage {
		return _meta;
	};
	blockBehaviors[BLOCK_SAPLING].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                          Java::Random& _random) -> void {
		// Add onto age
		if ((_meta & 0b1100) < 0b1100) {
			_meta += 0b100;
			_world.SetMeta(_pos, _meta);
			return;
		}
		// Remove the sapling so it doesn't interfere with the tree
		_world.SetBlock(_pos, BLOCK_AIR);
		// Remove sapling so the tree can grow in its place
		bool successfullyGrew = false;
		switch (TreeType(_meta & 0b11)) {
		case TreeType::Oak: // Oak or Large Oak
			if (_random.NextInt(10) == 0)
				successfullyGrew = TreeGenerator::BigTree().Generate(_world, _random, _pos);
			else
				successfullyGrew = TreeGenerator::GenerateTree(_world, _random, _pos);
			break;
		case TreeType::Spruce: // Spruce (lt Taiga)
			successfullyGrew = TreeGenerator::GenerateAltTaiga(_world, _random, _pos);
			break;
		case TreeType::Birch: // Birch
			successfullyGrew = TreeGenerator::GenerateTree(_world, _random, _pos, true);
			break;
		default:
			break;
		}
		// If the tree placement failed, just place the sapling back
		if (!successfullyGrew)
			_world.SetBlock(_pos, BLOCK_SAPLING, _meta);
	};
	blockBehaviors[BLOCK_SAPLING].damageDropped = [](uint8_t _meta) -> ItemDamage {
		return _meta & 3;
	};

	// --------------- don't drop anything ---------------
	blockBehaviors[BLOCK_ICE].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_GLASS].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_BOOKSHELF].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_CAKE].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_MOB_SPAWNER].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_FIRE].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_PISTON_HEAD].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_PISTON_MOVING].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_NETHER_PORTAL].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_SNOW_LAYER].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_WATER_FLOWING].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_WATER_STILL].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_LAVA_FLOWING].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_LAVA_STILL].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};

	// --------------- drops influenced by RNG ---------------
	blockBehaviors[BLOCK_GRAVEL].idDropped = [](uint8_t, Java::Random& _rng) -> ItemId {
		return _rng.NextInt(10) == 0 ? static_cast<ItemId>(Items::Id::FLINT) : static_cast<ItemId>(BLOCK_GRAVEL);
	};

	blockBehaviors[BLOCK_ORE_LAPIS_LAZULI].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::DYE;
	};
	blockBehaviors[BLOCK_ORE_LAPIS_LAZULI].damageDropped = [](uint8_t) -> ItemDamage {
		return 4;
	};
	blockBehaviors[BLOCK_ORE_LAPIS_LAZULI].quantityDropped = [](Java::Random& _rng) -> ItemAmount {
		return 4 + _rng.NextInt(5);
	};

	blockBehaviors[BLOCK_ORE_REDSTONE_OFF].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE;
	};
	blockBehaviors[BLOCK_ORE_REDSTONE_OFF].quantityDropped = [](Java::Random& _rng) -> ItemAmount {
		return 4 + _rng.NextInt(2);
	};
	blockBehaviors[BLOCK_ORE_REDSTONE_ON].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE;
	};
	blockBehaviors[BLOCK_ORE_REDSTONE_ON].quantityDropped = [](Java::Random& _rng) -> ItemAmount {
		return 4 + _rng.NextInt(2);
	};

	blockBehaviors[BLOCK_GLOWSTONE].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::GLOWSTONE_DUST;
	};
	blockBehaviors[BLOCK_GLOWSTONE].quantityDropped = [](Java::Random& _rng) -> ItemAmount {
		return 2 + _rng.NextInt(3);
	};
	blockBehaviors[BLOCK_LEAVES].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_SAPLING;
	};
	blockBehaviors[BLOCK_LEAVES].damageDropped = [](uint8_t _meta) -> ItemDamage {
		return _meta & 3;
	};
	blockBehaviors[BLOCK_LEAVES].quantityDropped = [](Java::Random& _rng) -> ItemAmount {
		return _rng.NextInt(20) == 0 ? 1 : 0;
	};

	blockBehaviors[BLOCK_TALLGRASS].idDropped = [](uint8_t, Java::Random& _rng) -> ItemId {
		return _rng.NextInt(8) == 0 ? Items::Id::SEEDS_WHEAT : Items::Id::INVALID;
	};

	// --------------- only drop if it's the correct half of the block being broken ---------------
	// TODO: other half of the block should be removed automatically
	blockBehaviors[BLOCK_DOOR_WOOD].idDropped = [](uint8_t _meta, Java::Random&) -> ItemId {
		return (_meta & 8) != 0 ? Items::Id::INVALID : Items::Id::DOOR_WOOD;
	};

	blockBehaviors[BLOCK_DOOR_IRON].idDropped = [](uint8_t _meta, Java::Random&) -> ItemId {
		return (_meta & 8) != 0 ? Items::Id::INVALID : Items::Id::DOOR_IRON;
	};

	blockBehaviors[BLOCK_BED].idDropped = [](uint8_t _meta, Java::Random&) -> ItemId {
		return (_meta & 8) != 0 ? Items::Id::INVALID : Items::Id::BED;
	};

	blockBehaviors[BLOCK_CLAY].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::CLAY;
	};
	blockBehaviors[BLOCK_CLAY].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 4;
	};

	blockBehaviors[BLOCK_SLAB].damageDropped = [](uint8_t _meta) -> ItemDamage {
		return _meta;
	};

	blockBehaviors[BLOCK_DOUBLE_SLAB].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_SLAB;
	};
	blockBehaviors[BLOCK_DOUBLE_SLAB].damageDropped = [](uint8_t _meta) -> ItemDamage {
		return _meta;
	};
	blockBehaviors[BLOCK_DOUBLE_SLAB].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 2;
	};

	blockBehaviors[BLOCK_SNOW].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::SNOWBALL;
	};
	blockBehaviors[BLOCK_SNOW].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 4;
	};
};

std::vector<ItemStack> GetBlockDrops(BlockType _blockId, uint8_t _meta, Java::Random& _rng) {
	std::vector<ItemStack> drops;

	if (_blockId == BLOCK_AIR) {
		return drops;
	}

	// headache: crops drop multiple items of different types (wheat + seeds)
	if (_blockId == BLOCK_CROP_WHEAT) {
		if (_meta == MAX_CROP_SIZE) {
			drops.push_back(ItemStack{ Items::Id::WHEAT, 1, 0 });
		}

		for (int i = 0; i < 3; i++) {
			if (_rng.NextInt(15) <= static_cast<int>(_meta)) {
				drops.push_back(ItemStack{ Items::Id::SEEDS_WHEAT, 1, 0 });
			}
		}

		return drops;
	}

	const BlockBehavior& behavior = blockBehaviors[static_cast<uint8_t>(_blockId)];
	int count = behavior.quantityDropped ? behavior.quantityDropped(_rng) : 1;
	int16_t damage = behavior.damageDropped ? behavior.damageDropped(_meta) : 0;

	for (int i = 0; i < count; i++) {
		ItemId id = behavior.idDropped ? behavior.idDropped(_meta, _rng) : ItemId(_blockId);

		if (id > 0) {
			drops.push_back(ItemStack{ id, 1, damage });
		}
	}

	return drops;
}

}; // namespace Blocks