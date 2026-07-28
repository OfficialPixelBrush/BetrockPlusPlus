/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "blocks/block_behaviors.h"
#include "blocks.h"
#include "blocks/block_properties.h"
#include "entities/entity_falling_block.h"
#include "enums/items.h"
#include "items/item_properties.h"
#include "logger.h"
#include "packet_data.h"
#include "tile_entities/tile_entity.h"
#include "world.h"

namespace Blocks {
BlockBehavior blockBehaviors[256] = {};

static Vec3 GetFluidFlowVector(WorldManager& _world, Int3 _pos) {
	auto waterMaterial = Material::Water();
	Vec3 flowVector{};
	auto getEffectiveFlowDecay = [&](WorldManager& _lWorld, Int3 _lPos, Material _lMaterial) {
		if (_lWorld.GetMaterial(_pos) != _lMaterial)
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

		if (!nearWall && isFluidWall({ _pos.x, _pos.y, _pos.z - 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ _pos.x, _pos.y, _pos.z + 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ _pos.x - 1, _pos.y, _pos.z }))
			nearWall = true;
		if (!nearWall && isFluidWall({ _pos.x + 1, _pos.y, _pos.z }))
			nearWall = true;
		if (!nearWall && isFluidWall({ _pos.x, _pos.y + 1, _pos.z - 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ _pos.x, _pos.y + 1, _pos.z + 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ _pos.x - 1, _pos.y + 1, _pos.z }))
			nearWall = true;
		if (!nearWall && isFluidWall({ _pos.x + 1, _pos.y + 1, _pos.z }))
			nearWall = true;

		if (nearWall) {
			// Normalize what we have so far, then let the huge -6 dominate the normalization after this
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

	// Final normalize
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

static bool GenericPlace(WorldManager& _world, Int3 _pos, [[maybe_unused]] Entity& _placer,
                         PacketData::FaceDirection _face, BlockType _blockId, uint8_t _meta) {
	BlockType existing = _world.GetBlockId(_pos);
	Int3 sourceBlock = Blocks::GetSourceBlockFromFace(_pos, _face);

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

	// Can we place this block here?
	auto entitiesInBlock = _world.entityManager.GetEntitiesWithinAabb(
	    { double(targetPos.x), double(targetPos.y), double(targetPos.z), double(targetPos.x) + 1.0,
	      double(targetPos.y) + 1.0, double(targetPos.z) + 1.0 });

	// Check to see if any entities overlap our block's collider
	if (Blocks::blockProperties[_blockId].isCollidable) {
		auto blockCollider = Blocks::blockBehaviors[_blockId].getCollider(_meta).Offset(
		    targetPos.x, targetPos.y,
		    targetPos.z); // Block colliders are at the origin so shift to world space
		for (auto& entity : entitiesInBlock) {
			if (blockCollider.Intersects(entity->collider) && entity->preventEntitySpawning)
				return false;
		}
	}

	_world.SetBlock(targetPos, _blockId, _meta);
	return true;
}

void RegisterBlockBehaviors() {
	// Initialize the default block placed behavior
	for (int i = 0; i < 256; i++) {
		blockBehaviors[i].onBlockPlaced = GenericPlace;
	}

	// Liquids/zero-size AABBs
	blockBehaviors[BlockType::BLOCK_WATER_FLOWING] = {
		.getSelectionBox = LiquidAabb,
		.getRayBounds = LiquidAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_WATER_STILL] = {
		.getSelectionBox = LiquidAabb,
		.getRayBounds = LiquidAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_LAVA_FLOWING] = {
		.getSelectionBox = LiquidAabb,
		.getRayBounds = LiquidAabb,
		.getCollider = EmptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_LAVA_STILL] = {
		.getSelectionBox = LiquidAabb,
		.getRayBounds = LiquidAabb,
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

	// specific behavioral overrides
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
	blockBehaviors[BLOCK_SUGARCANE].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos) -> void {
		// Check to see if our placement is still valid
		if (!CanSugarcaneSurviveAt(_world, _pos))
			BreakAndDropBlock(_world, _pos);
	};

	// placement overrides
	auto onFurnaceDispenserPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, PacketData::FaceDirection _face,
	                                  BlockType _blockId, uint8_t _meta) -> bool {
		int meta[] = { 2, 5, 3, 4 };
		return GenericPlace(_world, _pos, _placer, _face, _blockId, meta[GetDirectionFromYaw(_placer.rotationYaw, 4)]);
	};

	auto onStairPlace = [](WorldManager& _world, Int3 _pos, Entity& _placer, PacketData::FaceDirection _face,
	                       BlockType _blockId, uint8_t _meta) -> bool {
		int meta[] = { 2, 1, 3, 0 };
		return GenericPlace(_world, _pos, _placer, _face, _blockId, meta[GetDirectionFromYaw(_placer.rotationYaw, 4)]);
	};

	blockBehaviors[BLOCK_FURNACE].onBlockPlaced = onFurnaceDispenserPlace;
	blockBehaviors[BLOCK_FURNACE_LIT].onBlockPlaced = onFurnaceDispenserPlace;
	blockBehaviors[BLOCK_DISPENSER].onBlockPlaced = onFurnaceDispenserPlace;

	blockBehaviors[BLOCK_STAIRS_COBBLESTONE].onBlockPlaced = onStairPlace;
	blockBehaviors[BLOCK_STAIRS_WOOD].onBlockPlaced = onStairPlace;
	blockBehaviors[BLOCK_LADDER].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                                PacketData::FaceDirection _face, BlockType _blockId,
	                                                uint8_t _meta) -> bool {
		Int3 targetPos = _pos;
		Int3 sourceBlock = Blocks::GetSourceBlockFromFace(_pos, _face);
		if (_world.GetBlockId(sourceBlock) == BLOCK_SNOW_LAYER)
			targetPos = sourceBlock;

		auto hasSupport = [&](PacketData::FaceDirection _dir) {
			Int3 support = GetAdjacentBlockPos(targetPos, PacketData::OppositeFace(_dir));
			return _world.IsBlockNormalCube(support);
		};

		static constexpr std::array<PacketData::FaceDirection, 4> CHECK_ORDER = { PacketData::FaceDirection::Z_MINUS,
			                                                                      PacketData::FaceDirection::Z_PLUS,
			                                                                      PacketData::FaceDirection::X_MINUS,
			                                                                      PacketData::FaceDirection::X_PLUS };

		if (_face >= 2 && hasSupport(_face)) {
			return GenericPlace(_world, _pos, _placer, _face, _blockId, uint8_t(_face));
		}

		for (PacketData::FaceDirection dir : CHECK_ORDER) {
			if (hasSupport(dir)) {
				return GenericPlace(_world, _pos, _placer, _face, _blockId, uint8_t(dir));
			}
		}
		return false;
	};

	blockBehaviors[BLOCK_TORCH].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                               PacketData::FaceDirection _face, BlockType _blockId,
	                                               uint8_t _meta) -> bool {
		if (_world.GetBlockId(Blocks::GetSourceBlockFromFace(_pos, _face)) == BLOCK_SNOW_LAYER)
			_pos = Blocks::GetSourceBlockFromFace(_pos, _face);
		if (CanTorchAttachTo(_world, _pos, _face)) {
			return GenericPlace(_world, _pos, _placer, _face, _blockId, 6 - _face);
		}
		return false;
	};

	blockBehaviors[BLOCK_TORCH].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		// This prevents floating torches from existing

		// Matches vanilla order
		static constexpr std::array<PacketData::FaceDirection, 5> CHECK_ORDER = {
			PacketData::FaceDirection::X_PLUS, PacketData::FaceDirection::X_MINUS, PacketData::FaceDirection::Z_PLUS,
			PacketData::FaceDirection::Z_MINUS, PacketData::FaceDirection::Y_PLUS
		};

		for (PacketData::FaceDirection face : CHECK_ORDER) {
			if (CanTorchAttachTo(_world, _pos, face)) {
				_world.SetMeta(_pos, 6 - face);
				return;
			}
		}

		BreakAndDropBlock(_world, _pos);
	};

	blockBehaviors[BLOCK_TORCH].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos) -> void {
		uint8_t meta = _world.GetMetadata(_pos);
		auto supportFace = static_cast<PacketData::FaceDirection>(6 - meta);
		if (!CanTorchAttachTo(_world, _pos, supportFace))
			BreakAndDropBlock(_world, _pos);
	};

	// for when the block is interacted with!
	blockBehaviors[BLOCK_DOOR_WOOD].onBlockActivated = [](WorldManager& _world, Int3 _pos) -> bool {
		auto meta = _world.GetMetadata(_pos);
		if (meta & 8) {
			// We are the top half of the door
			if (_world.GetBlockId({ _pos.x, _pos.y - 1, _pos.z }) != BLOCK_DOOR_WOOD)
				// Below us is not the bottom of a door! This is bad!
				return false;
			// Recall this function on the bottom of the door
			blockBehaviors[BLOCK_DOOR_WOOD].onBlockActivated(_world, { _pos.x, _pos.y - 1, _pos.z });
			return false;
		}
		// We are the top half so lets open
		Int3 top = { _pos.x, _pos.y + 1, _pos.z };
		if (_world.GetBlockId(top) == BLOCK_DOOR_WOOD && (_world.GetMetadata(top) & 8)) {
			_world.SetMeta(top, uint8_t((meta ^ 4) + 8));
		}
		_world.SetMeta(_pos, uint8_t(meta ^ 4)); // XOR bit 2; flips open/closed
		return false;
	};

	// Falling blocks!
	blockBehaviors[BLOCK_GRAVEL].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos) -> void {
		// Schedule a check to see if we can fall
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_GRAVEL, 3);
	};
	blockBehaviors[BLOCK_GRAVEL].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_GRAVEL, 3);
	};
	blockBehaviors[BLOCK_GRAVEL].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta,
	                                         Java::Random& _random) -> void {
		Int3 below = { _pos.x, _pos.y - 1, _pos.z };

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
			while (Blocks::CanFallAt(_world, { landing.x, landing.y - 1, landing.z }) && landing.y > 0)
				landing.y--;

			if (landing.y > 0)
				_world.SetBlock(landing, BLOCK_GRAVEL, 0);
		}
	};
	blockBehaviors[BLOCK_SAND].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos) -> void {
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
			Vec3 spawnPos = { _pos.x + 0.5, _pos.y + 0.5, _pos.z + 0.5 };
			auto entity = std::make_shared<FallingBlockEntity>(spawnPos, BLOCK_SAND);
			_world.entityManager.AddEntity(std::move(entity));
			_world.SetBlock(_pos, BLOCK_AIR, 0);
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
	blockBehaviors[BLOCK_FURNACE].onBlockRemoval = [](WorldManager& _world, Int3 _pos) -> void {
		auto* te = _world.GetTileEntityAs<TileEntityFurnace>(_pos);
		if (!te)
			return;

		_world.DropInventory(te->inventory, _pos);
	};

	//TODO: Add another portal creation function matching with b1.7.3's limitations, that is toggleable via a config entry,
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

	blockBehaviors[BLOCK_SIGN].idDropped = [](uint8_t, Java::Random&) -> ItemId {
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