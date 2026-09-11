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

void RegisterFluidBehaviors() {
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
	// Lava physics
	blockBehaviors[BLOCK_LAVA_FLOWING].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		// Schedule ourselves for an update
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_LAVA_FLOWING,
		                                        _world.GetDimension() == Dimension::Nether ? 10 : 30);
		TryLavaHarden(_world, _pos);
	};
	blockBehaviors[BLOCK_LAVA_FLOWING].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                              BlockType /*_blockId*/) -> void {
		// Stack overflow if this was regular set block!
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_LAVA_FLOWING,
		                                        _world.GetDimension() == Dimension::Nether ? 10 : 30);
		TryLavaHarden(_world, _pos);
	};
	blockBehaviors[BLOCK_LAVA_STILL].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                            BlockType /*_blockId*/) -> void {
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
	                                               Java::Random& /*_random*/) -> void {
		auto level = _meta % 8;
		bool isFalling = _meta >= 8;
		bool isSource = _meta == 0;
		auto candidateLevel = -1;
		Int3 belowPos = { _pos.x, _pos.y - 1, _pos.z };

		int stepDecay = _world.GetDimension() == Dimension::Nether ? 1 : 2;

		// Are we a source block?
		if (!isSource) {
			// We aren't a source block so we need to update our level
			// TODO: adjacentSourceCount isn't used?
			//int8_t adjacentSourceCount = 0;
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
					//if (neighborLevel == 0)
					//	adjacentSourceCount++;
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
	                                                               BlockType /*_blockId*/) -> void {
		// Stack overflow if this was regular set block!
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_WATER_FLOWING, 5);
	};
	blockBehaviors[BLOCK_WATER_STILL].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos,
	                                                             BlockType /*_blockId*/) -> void {
		// Stack overflow if this was regular set block!
		_world.SetBlockRaw(_pos, BLOCK_WATER_FLOWING, _world.GetMetadata(_pos));
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_WATER_FLOWING, 5);
	};
	blockBehaviors[BLOCK_WATER_FLOWING].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                                Java::Random& /*_random*/) -> void {
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
}

}; // namespace Blocks
