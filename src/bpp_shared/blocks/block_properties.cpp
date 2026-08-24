/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "block_properties.h"
#include "blocks.h"
#include "entities/entity_item.h"
#include "packet_data.h"
#include "world/world.h"
#include <cstdint>

namespace Blocks {

// Global table definitions; declared extern in the header
BlockProperties blockProperties[BLOCK_MAX] = {};

// Behavior helper functions
bool CanFallAt(WorldAccess& _world, Int3 _position) {
	auto block = _world.GetBlockId(_position);
	if (block == BLOCK_AIR)
		return true;
	if (block == BLOCK_FIRE)
		return true;
	auto material = Blocks::blockProperties[block].material.type;
	if (material == MaterialType::Lava || material == MaterialType::Water)
		return true;
	return false;
}

void BreakAndDropBlock(WorldManager& _world, Int3 _pos) {
	BlockType blockId = _world.GetBlockId({ _pos.x, _pos.y, _pos.z });
	if (blockId == BLOCK_AIR)
		return;
	uint8_t meta = _world.GetMetadata({ _pos.x, _pos.y, _pos.z });
	_world.SetBlock({ _pos.x, _pos.y, _pos.z }, BLOCK_AIR);

	std::vector<ItemStack> drops = Blocks::GetBlockDrops(blockId, meta, _world.rand);

	for (ItemStack drop : drops) {
		Vec3 dropPos = { double(_pos.x), double(_pos.y), double(_pos.z) };
		float offset = 0.7f;
		dropPos.x += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
		dropPos.y += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
		dropPos.z += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
		ItemEntity item(dropPos);
		item.itemStack = drop;
		_world.entityManager.AddEntity(std::make_shared<ItemEntity>(item));
	}
	return;
}

void BreakAndDropBlockWithChance(WorldManager& _world, Int3 _pos, float _chance) {
	BlockType blockId = _world.GetBlockId({ _pos.x, _pos.y, _pos.z });
	if (blockId == BLOCK_AIR)
		return;
	uint8_t meta = _world.GetMetadata({ _pos.x, _pos.y, _pos.z });
	_world.SetBlock({ _pos.x, _pos.y, _pos.z }, BLOCK_AIR);

	std::vector<ItemStack> drops = Blocks::GetBlockDrops(blockId, meta, _world.rand);

	for (ItemStack drop : drops) {
		if (_world.rand.NextFloat() <= _chance) {
			Vec3 dropPos = { double(_pos.x), double(_pos.y), double(_pos.z) };
			float offset = 0.7f;
			dropPos.x += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
			dropPos.y += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
			dropPos.z += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
			ItemEntity item(dropPos);
			item.itemStack = drop;
			_world.entityManager.AddEntity(std::make_shared<ItemEntity>(item));
		}
	}
	return;
}

void DropBlockAt(WorldManager& _world, Int3 _pos, BlockType _id, ItemAmount _count, int16_t _data) {
	Vec3 dropPos = { double(_pos.x), double(_pos.y), double(_pos.z) };
	float offset = 0.7f;
	dropPos.x += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
	dropPos.y += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
	dropPos.z += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
	ItemEntity item(dropPos);
	item.itemStack = { .id = _id, .count = _count, .data = _data };
	_world.entityManager.AddEntity(std::make_shared<ItemEntity>(item));
	return;
}

void DropItemAt(WorldManager& _world, Int3 _pos, Items::Id _id, ItemAmount _count, int16_t _data) {
	Vec3 dropPos = { double(_pos.x), double(_pos.y), double(_pos.z) };
	float offset = 0.7f;
	dropPos.x += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
	dropPos.y += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
	dropPos.z += (_world.rand.NextFloat() * offset) + (1.0f - offset) * 0.5;
	ItemEntity item(dropPos);
	item.itemStack = { .id = _id, .count = _count, .data = _data };
	_world.entityManager.AddEntity(std::make_shared<ItemEntity>(item));
	return;
}

bool CanSugarcaneSurviveAt(WorldAccess& _world, Int3 _pos) {
	auto belowBlock = _world.GetBlockId({ _pos.x, _pos.y - 1, _pos.z });
	if (belowBlock == BLOCK_SUGARCANE)
		return true;
	if (belowBlock != BLOCK_GRASS && belowBlock != BLOCK_DIRT)
		return false;
	// Check for water
	int d[4] = { -1, 1, 0, 0 };
	for (int i = 0; i < 4; i++) {
		int dx = d[i];
		int dz = d[3 - i];
		auto adjacentBlock = _world.GetBlockId({ _pos.x + dx, _pos.y - 1, _pos.z + dz });
		if (adjacentBlock == BLOCK_WATER_FLOWING || adjacentBlock == BLOCK_WATER_STILL)
			return true;
	}
	return false;
}

bool CanCropsSurviveAt(WorldAccess& _world, Int3 _pos) {
	auto belowBlock = _world.GetBlockId({ _pos.x, _pos.y - 1, _pos.z });
	return belowBlock == BLOCK_FARMLAND;
}

bool CanGenericPlantSurviveAt(WorldAccess& _world, Int3 _pos) {
	auto lightLevel = _world.GetBlockLightRaw(_pos);
	bool canSeeSky = _world.CanBlockSeeSky(_pos);
	auto belowBlock = _world.GetBlockId({ _pos.x, _pos.y - 1, _pos.z });
	bool canGrowOnBlock = belowBlock == BLOCK_FARMLAND || belowBlock == BLOCK_DIRT || belowBlock == BLOCK_GRASS;
	return (lightLevel >= 8 || canSeeSky) && canGrowOnBlock;
}

bool CanMushroomSurviveAt(WorldAccess& _world, Int3 _pos) {
	auto lightLevel = _world.GetBlockLightRaw(_pos);
	auto belowBlock = _world.GetBlockId({ _pos.x, _pos.y - 1, _pos.z });
	bool canGrowOnBlock = blockProperties[belowBlock].isOpaqueCube;
	return lightLevel <= 13 && canGrowOnBlock;
}

bool CanCactusSurviveAt(WorldManager& _world, Int3 _pos) {
	bool adjacentBlocksClear = true;
	int d[4] = { -1, 1, 0, 0 };
	for (int i = 0; i < 4; i++) {
		int dx = _pos.x + d[i];
		int dz = _pos.z + d[3 - i];
		if (_world.GetMaterial({ dx, _pos.y, dz }).isSolid) {
			adjacentBlocksClear = false;
			break;
		}
	}
	auto belowBlock = _world.GetBlockId({ _pos.x, _pos.y - 1, _pos.z });
	bool canGrowOnBlock = belowBlock == BLOCK_CACTUS || belowBlock == BLOCK_SAND;
	return canGrowOnBlock && adjacentBlocksClear;
}

bool CanTorchAttachTo(WorldManager& _world, Int3 _pos, PacketData::FaceDirection _face) {
	if (_face == PacketData::FaceDirection::Y_MINUS)
		return false;

	Int3 support = GetAdjacentBlockPos(_pos, OppositeFace(_face));

	return _world.IsBlockNormalCube(support) ||
	       (_face == PacketData::FaceDirection::Y_PLUS && _world.GetBlockId(support) == BLOCK_FENCE);
}

// Some fluid specific stuff
float GetFluidPercentAir(uint8_t _meta) {
	if (_meta >= 8)
		_meta = 0;

	return float(_meta + 1) / 9.0f;
}

void RegisterBlockProperties() {
	// Air
	blockProperties[BlockType::BLOCK_AIR] = {
		.material = Material::Air(),
		.lightOpacity = 0,
		.hardness = 0.0f,
		.resistance = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.canBlockGrass = false,
		.enableStats = false,
	};

	// Stone
	blockProperties[BlockType::BLOCK_STONE] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 1.5f,
		.resistance = 10.0f,
	};

	// Grass
	blockProperties[BlockType::BLOCK_GRASS] = {
		.material = Material::Grass(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 255,
		.hardness = 0.6f,
		.ticksOnLoad = true,
	};

	// Dirt
	blockProperties[BlockType::BLOCK_DIRT] = {
		.material = Material::Ground(),
		.stepSound = StepSound::Gravel,
		.lightOpacity = 255,
		.hardness = 0.5f,
	};

	// Cobblestone
	blockProperties[BlockType::BLOCK_COBBLESTONE] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 2.0f,
		.resistance = 10.0f,
	};

	// Planks (Oak Wood)
	blockProperties[BlockType::BLOCK_PLANKS] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 255,
		.hardness = 2.0f,
		.resistance = 5.0f,
	};

	// Sapling
	blockProperties[BlockType::BLOCK_SAPLING] = {
		.material = Material::Plants(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.ticksOnLoad = true,
	};

	// Bedrock
	blockProperties[BlockType::BLOCK_BEDROCK] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.hardness = -1.0f, // unbreakable
		.resistance = 6000000.0f,
		.enableStats = false,
	};

	// Water (flowing)
	blockProperties[BlockType::BLOCK_WATER_FLOWING] = {
		.material = Material::Water(),
		.lightOpacity = 3,
		.hardness = 100.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Water (still/stationary)
	blockProperties[BlockType::BLOCK_WATER_STILL] = {
		.material = Material::Water(),
		.lightOpacity = 3,
		.hardness = 100.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Lava (flowing)
	blockProperties[BlockType::BLOCK_LAVA_FLOWING] = {
		.material = Material::Lava(),
		.lightEmission = 15, // setLightValue(1.0f) -> 15*1.0 = 15
		.lightOpacity = 255,
		.hardness = 0.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Lava (still/stationary)
	blockProperties[BlockType::BLOCK_LAVA_STILL] = {
		.material = Material::Lava(),
		.lightEmission = 15,
		.lightOpacity = 255,
		.hardness = 100.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Sand
	blockProperties[BlockType::BLOCK_SAND] = {
		.material = Material::Sand(),
		.stepSound = StepSound::Sand,
		.lightOpacity = 255,
		.hardness = 0.5f,
	};

	// Gravel
	blockProperties[BlockType::BLOCK_GRAVEL] = {
		.material = Material::Sand(),
		.stepSound = StepSound::Gravel,
		.lightOpacity = 255,
		.hardness = 0.6f,
	};

	// Gold Ore
	blockProperties[BlockType::BLOCK_ORE_GOLD] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 3.0f,
		.resistance = 5.0f,
	};

	// Iron Ore
	blockProperties[BlockType::BLOCK_ORE_IRON] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 3.0f,
		.resistance = 5.0f,
	};

	// Coal Ore
	blockProperties[BlockType::BLOCK_ORE_COAL] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 3.0f,
		.resistance = 5.0f,
	};

	// Wood Log
	blockProperties[BlockType::BLOCK_LOG] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 255,
		.hardness = 2.0f,
	};

	// Leaves
	blockProperties[BlockType::BLOCK_LEAVES] = {
		.material = Material::Leaves(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 1,
		.hardness = 0.2f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.ticksOnLoad = true,
		.enableStats = false,
	};

	// Sponge
	blockProperties[BlockType::BLOCK_SPONGE] = {
		.material = Material::Sponge(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 255,
		.hardness = 0.6f,
	};

	// Glass
	blockProperties[BlockType::BLOCK_GLASS] = {
		.material = Material::Glass(),
		.stepSound = StepSound::Glass,
		.lightOpacity = 0,
		.hardness = 0.3f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Lapis Lazuli Ore
	blockProperties[BlockType::BLOCK_ORE_LAPIS_LAZULI] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 3.0f,
		.resistance = 5.0f,
	};

	// Lapis Lazuli Block
	blockProperties[BlockType::BLOCK_LAPIS_LAZULI] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 3.0f,
		.resistance = 5.0f,
	};

	// Dispenser
	blockProperties[BlockType::BLOCK_DISPENSER] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 3.5f,
	};

	// Sandstone
	blockProperties[BlockType::BLOCK_SANDSTONE] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 0.8f,
	};

	// Note Block
	blockProperties[BlockType::BLOCK_NOTEBLOCK] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 255,
		.hardness = 0.8f,
	};

	// Bed
	blockProperties[BlockType::BLOCK_BED] = {
		.material = Material::Cloth(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 0.2f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Powered Rail (Golden Rail)
	blockProperties[BlockType::BLOCK_RAIL_POWERED] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 0,
		.hardness = 0.7f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Detector Rail
	blockProperties[BlockType::BLOCK_RAIL_DETECTOR] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 0,
		.hardness = 0.7f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Sticky Piston Base
	blockProperties[BlockType::BLOCK_PISTON_STICKY] = {
		.material = Material::Piston(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 0.5f,
		.isOpaqueCube = false,
	};

	// Cobweb
	blockProperties[BlockType::BLOCK_COBWEB] = {
		.material = Material::Web(),
		.stepSound = StepSound::Cloth,
		.lightOpacity = 1,
		.hardness = 4.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Tall Grass
	blockProperties[BlockType::BLOCK_TALLGRASS] = {
		.material = Material::Plants(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Dead Bush
	blockProperties[BlockType::BLOCK_DEADBUSH] = {
		.material = Material::Plants(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Piston Base
	blockProperties[BlockType::BLOCK_PISTON] = {
		.material = Material::Piston(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 0.5f,
		.isOpaqueCube = false,
	};

	// Piston Extension (head)
	blockProperties[BlockType::BLOCK_PISTON_HEAD] = {
		.material = Material::Piston(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 0,
		.hardness = 0.5f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Wool (Cloth)
	blockProperties[BlockType::BLOCK_WOOL] = {
		.material = Material::Cloth(),
		.stepSound = StepSound::Cloth,
		.lightOpacity = 255,
		.hardness = 0.8f,
	};

	// Piston Moving (tile entity placeholder)
	blockProperties[BlockType::BLOCK_PISTON_MOVING] = {
		.material = Material::Piston(),
		.lightOpacity = 0,
		.hardness = -1.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Dandelion (Yellow Flower)
	blockProperties[BlockType::BLOCK_DANDELION] = {
		.material = Material::Plants(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Rose (Red Flower)
	blockProperties[BlockType::BLOCK_ROSE] = {
		.material = Material::Plants(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Brown Mushroom
	blockProperties[BlockType::BLOCK_MUSHROOM_BROWN] = {
		.material = Material::Plants(),
		.stepSound = StepSound::Grass,
		.lightEmission = 1,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Red Mushroom
	blockProperties[BlockType::BLOCK_MUSHROOM_RED] = {
		.material = Material::Plants(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Gold Block
	blockProperties[BlockType::BLOCK_GOLD] = {
		.material = Material::Iron(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 3.0f,
		.resistance = 10.0f,
	};

	// Iron Block
	blockProperties[BlockType::BLOCK_IRON] = {
		.material = Material::Iron(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 5.0f,
		.resistance = 10.0f,
	};

	// Double Stone Slab
	blockProperties[BlockType::BLOCK_DOUBLE_SLAB] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 2.0f,
		.resistance = 10.0f,
	};

	// Stone Slab (single)
	blockProperties[BlockType::BLOCK_SLAB] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 2.0f,
		.resistance = 10.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Bricks
	blockProperties[BlockType::BLOCK_BRICKS] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 2.0f,
		.resistance = 10.0f,
	};

	// TNT
	blockProperties[BlockType::BLOCK_TNT] = {
		.material = Material::TNT(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 255,
		.hardness = 0.0f,
	};

	// Bookshelf
	blockProperties[BlockType::BLOCK_BOOKSHELF] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 255,
		.hardness = 1.5f,
	};

	// Mossy Cobblestone
	blockProperties[BlockType::BLOCK_COBBLESTONE_MOSSY] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 2.0f,
		.resistance = 10.0f,
	};

	// Obsidian
	blockProperties[BlockType::BLOCK_OBSIDIAN] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 10.0f,
		.resistance = 2000.0f,
	};

	// Torch
	blockProperties[BlockType::BLOCK_TORCH] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Wood,
		.lightEmission = 14,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Fire
	blockProperties[BlockType::BLOCK_FIRE] = {
		.material = Material::Fire(),
		.lightEmission = 15,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.canBlockGrass = false,
		.enableStats = false,
	};

	// Monster Spawner
	blockProperties[BlockType::BLOCK_MOB_SPAWNER] = {
		.material = Material::Iron(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 0,
		.hardness = 5.0f,
		.enableStats = false,
	};

	// Oak Wood Stairs
	blockProperties[BlockType::BLOCK_STAIRS_WOOD] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 255,
		.hardness = 2.0f,
		.resistance = 5.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Chest
	blockProperties[BlockType::BLOCK_CHEST] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 2.5f,
		.isOpaqueCube = false,
		.isNormalCube = false,
	};

	// Redstone Wire
	blockProperties[BlockType::BLOCK_REDSTONE] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Diamond Ore
	blockProperties[BlockType::BLOCK_ORE_DIAMOND] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 3.0f,
		.resistance = 5.0f,
	};

	// Diamond Block
	blockProperties[BlockType::BLOCK_DIAMOND] = {
		.material = Material::Iron(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 5.0f,
		.resistance = 10.0f,
	};

	// Crafting Table (Workbench)
	blockProperties[BlockType::BLOCK_CRAFTING_TABLE] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 255,
		.hardness = 2.5f,
	};

	// Crops / Wheat
	blockProperties[BlockType::BLOCK_CROP_WHEAT] = {
		.material = Material::Plants(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.ticksOnLoad = true,
		.enableStats = false,
	};

	// Farmland (Tilled Field)
	blockProperties[BlockType::BLOCK_FARMLAND] = {
		.material = Material::Ground(),
		.stepSound = StepSound::Gravel,
		.lightOpacity = 255,
		.hardness = 0.6f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Furnace (idle)
	blockProperties[BlockType::BLOCK_FURNACE] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 3.5f,
	};

	// Furnace (active/lit)
	blockProperties[BlockType::BLOCK_FURNACE_LIT] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightEmission = 13,
		.lightOpacity = 255,
		.hardness = 3.5f,
	};

	// Sign (standing)
	blockProperties[BlockType::BLOCK_SIGN] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 1.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Wooden Door
	blockProperties[BlockType::BLOCK_DOOR_WOOD] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 3.0f,
		.resistance = 3.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Ladder
	blockProperties[BlockType::BLOCK_LADDER] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 0.4f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Rail (normal)
	blockProperties[BlockType::BLOCK_RAIL] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 0,
		.hardness = 0.7f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Cobblestone Stairs
	blockProperties[BlockType::BLOCK_STAIRS_COBBLESTONE] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 2.0f,
		.resistance = 10.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Wall Sign
	blockProperties[BlockType::BLOCK_SIGN_WALL] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 1.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Lever
	blockProperties[BlockType::BLOCK_LEVER] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 0.5f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Stone Pressure Plate
	blockProperties[BlockType::BLOCK_PRESSURE_PLATE_STONE] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 0,
		.hardness = 0.5f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Iron Door
	blockProperties[BlockType::BLOCK_DOOR_IRON] = {
		.material = Material::Iron(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 0,
		.hardness = 5.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Wooden Pressure Plate
	blockProperties[BlockType::BLOCK_PRESSURE_PLATE_WOOD] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 0.5f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Redstone Ore
	blockProperties[BlockType::BLOCK_ORE_REDSTONE_OFF] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 3.0f,
		.resistance = 5.0f,
	};

	// Redstone Ore (glowing/lit)
	blockProperties[BlockType::BLOCK_ORE_REDSTONE_ON] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightEmission = 9,
		.lightOpacity = 255,
		.hardness = 3.0f,
		.resistance = 5.0f,
	};

	// Redstone Torch (off)
	blockProperties[BlockType::BLOCK_REDSTONE_TORCH_OFF] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Redstone Torch (on)
	blockProperties[BlockType::BLOCK_REDSTONE_TORCH_ON] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Wood,
		.lightEmission = 7,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Stone Button
	blockProperties[BlockType::BLOCK_BUTTON_STONE] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 0,
		.hardness = 0.5f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Snow (layer)
	blockProperties[BlockType::BLOCK_SNOW_LAYER] = {
		.material = Material::SnowLayer(),
		.stepSound = StepSound::Cloth,
		.lightOpacity = 0,
		.hardness = 0.1f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.canBlockGrass = false,
	};

	// Ice
	blockProperties[BlockType::BLOCK_ICE] = {
		.material = Material::Ice(),
		.stepSound = StepSound::Glass,
		.lightOpacity = 3,
		.hardness = 0.5f,
		.slipperiness = 0.98f,
	};

	// Snow Block
	blockProperties[BlockType::BLOCK_SNOW] = {
		.material = Material::SnowBlock(),
		.stepSound = StepSound::Cloth,
		.lightOpacity = 255,
		.hardness = 0.2f,
	};

	// Cactus
	blockProperties[BlockType::BLOCK_CACTUS] = {
		.material = Material::Cactus(),
		.stepSound = StepSound::Cloth,
		.lightOpacity = 0,
		.hardness = 0.4f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.ticksOnLoad = true,
	};

	// Clay Block
	blockProperties[BlockType::BLOCK_CLAY] = {
		.material = Material::Clay(),
		.stepSound = StepSound::Gravel,
		.lightOpacity = 255,
		.hardness = 0.6f,
	};

	// Sugar Cane (Reed)
	blockProperties[BlockType::BLOCK_SUGARCANE] = {
		.material = Material::Plants(),
		.stepSound = StepSound::Grass,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.ticksOnLoad = true,
		.enableStats = false,
	};

	// Jukebox
	blockProperties[BlockType::BLOCK_JUKEBOX] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 2.0f,
		.resistance = 10.0f,
	};

	// Fence
	blockProperties[BlockType::BLOCK_FENCE] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 2.0f,
		.resistance = 5.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Pumpkin
	blockProperties[BlockType::BLOCK_PUMPKIN] = {
		.material = Material::Pumpkin(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 255,
		.hardness = 1.0f,
	};

	// Netherrack
	blockProperties[BlockType::BLOCK_NETHERRACK] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 0.4f,
	};

	// Soul Sand
	blockProperties[BlockType::BLOCK_SOULSAND] = {
		.material = Material::Sand(),
		.stepSound = StepSound::Sand,
		.lightOpacity = 255,
		.hardness = 0.5f,
	};

	// Glowstone
	blockProperties[BlockType::BLOCK_GLOWSTONE] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Glass,
		.lightEmission = 15,
		.lightOpacity = 255,
		.hardness = 0.3f,
	};

	// Nether Portal
	blockProperties[BlockType::BLOCK_NETHER_PORTAL] = {
		.material = Material::Portal(),
		.stepSound = StepSound::Glass,
		.lightEmission = 11,
		.lightOpacity = 0,
		.hardness = -1.0f,
		.isCollidable = false,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
	};

	// Jack-o-Lantern (Lit Pumpkin)
	blockProperties[BlockType::BLOCK_PUMPKIN_LIT] = {
		.material = Material::Pumpkin(),
		.stepSound = StepSound::Wood,
		.lightEmission = 15,
		.lightOpacity = 255,
		.hardness = 1.0f,
	};

	// Cake
	blockProperties[BlockType::BLOCK_CAKE] = {
		.material = Material::Cake(),
		.stepSound = StepSound::Cloth,
		.lightOpacity = 0,
		.hardness = 0.5f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Redstone Repeater (off)
	blockProperties[BlockType::BLOCK_REDSTONE_REPEATER_OFF] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Redstone Repeater (on)
	blockProperties[BlockType::BLOCK_REDSTONE_REPEATER_ON] = {
		.material = Material::Circuits(),
		.stepSound = StepSound::Wood,
		.lightEmission = 9,
		.lightOpacity = 0,
		.hardness = 0.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};

	// Trapdoor
	blockProperties[BlockType::BLOCK_TRAPDOOR] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 3.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.enableStats = false,
	};
}

} // namespace Blocks