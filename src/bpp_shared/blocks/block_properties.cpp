/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "block_properties.h"
#include "enums/items.h"
#include "tile_entities/tile_entity.h"
#include "entities/entity_item.h"
#include "world/world.h"

namespace Blocks {

// Global table definitions; declared extern in the header
BlockProperties blockProperties[256] = {};
BlockBehavior blockBehaviors[256] = {};

// Behavior helper functions
void BreakAndDropBlock(WorldManager& world, Int3& pos) {
	BlockType blockId = world.getBlockId({ pos.x, pos.y, pos.z });
	uint8_t meta = world.getMetadata({ pos.x, pos.y, pos.z });
	world.setBlock({ pos.x, pos.y, pos.z }, BLOCK_AIR);

	std::vector<ItemStack> drops = Blocks::getBlockDrops(blockId, meta, world.rand);

	for (ItemStack drop : drops) {
		Vec3 dropPos = { double(pos.x), double(pos.y), double(pos.z) };
		float offset = 0.7f;
		dropPos.x += (world.rand.nextFloat() * offset) + (1.0f - offset) * 0.5;
		dropPos.y += (world.rand.nextFloat() * offset) + (1.0f - offset) * 0.5;
		dropPos.z += (world.rand.nextFloat() * offset) + (1.0f - offset) * 0.5;
		ItemEntity item(dropPos);
		item.itemStack = drop;
		world.entityManager.addEntity(std::make_shared<ItemEntity>(item));
	}
	return;
}

// Some fluid specific stuff
float getFluidPercentAir(uint8_t meta) {
	if (meta >= 8)
		meta = 0;

	return float(meta + 1) / 9.0f;
}

static Vec3 getFluidFlowVector(WorldManager& world, Int3 pos) {
	auto waterMaterial = Material::Water();
	Vec3 flowVector{};
	auto getEffectiveFlowDecay = [&](WorldManager& lWorld, Int3 lPos, Material lMaterial) {
		if (lWorld.getMaterial(pos) != lMaterial)
			return -1;
		int meta = lWorld.getMetadata(lPos);
		if (meta >= 8)
			meta = 0;

		return meta;
	};

	int myFlowContribution = getEffectiveFlowDecay(world, pos, waterMaterial);

	// Get the contribution of our horizontal neighbors
	int ndx[] = { -1, 1, 0, 0 };
	int ndz[] = { 0, 0, -1, 1 };
	for (int i = 0; i < 4; i++) {
		int dx = pos.x + ndx[i];
		int dz = pos.z + ndz[i];
		int neighborFlowContribution = getEffectiveFlowDecay(world, { dx, pos.y, dz }, waterMaterial);
		int flowDifference = 0;
		// Our neighbor block didn't have the same material
		if (neighborFlowContribution < 0) {
			if (!world.getMaterial({ dx, pos.y, dz }).isSolid) {
				// Check the block below us to see if its water, if it is, STRONGLY pull down
				int belowFlowContribution = getEffectiveFlowDecay(world, { dx, pos.y - 1, dz }, waterMaterial);
				if (belowFlowContribution >= 0) {
					flowDifference = belowFlowContribution - (myFlowContribution - 8);
					flowVector.x += double((dx - pos.x) * flowDifference);
					flowVector.z += double((dz - pos.z) * flowDifference);
				}
			}
		} else {
			flowDifference = neighborFlowContribution - myFlowContribution;
			flowVector.x += double((dx - pos.x) * flowDifference);
			flowVector.z += double((dz - pos.z) * flowDifference);
		}
	}

	auto isFluidWall = [&](Int3 checkPos) {
		Material neighborMaterial = world.getMaterial(checkPos);
		if (neighborMaterial == waterMaterial)
			return false;
		if (neighborMaterial == Material::Ice())
			return false;
		return neighborMaterial.isSolid;
	};

	// If we're a falling fluid segment, check whether we're clinging to a wall
	if (world.getMetadata(pos) >= 8) {
		bool nearWall = false;

		if (!nearWall && isFluidWall({ pos.x, pos.y, pos.z - 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.x, pos.y, pos.z + 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.x - 1, pos.y, pos.z }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.x + 1, pos.y, pos.z }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.x, pos.y + 1, pos.z - 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.x, pos.y + 1, pos.z + 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.x - 1, pos.y + 1, pos.z }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.x + 1, pos.y + 1, pos.z }))
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

// defaults
static AABB defaultAABB(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
}
static CollisionShape defaultCollider(uint8_t) {
	CollisionShape s;
	s.add({ 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 });
	return s;
}

// slab
static AABB slabAABB(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 };
}
static CollisionShape slabCollider(uint8_t) {
	CollisionShape s;
	s.add({ 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 });
	return s;
}

// stairs
static CollisionShape stairCollider(uint8_t meta) {
	CollisionShape s;
	switch (meta & 3) {
	case 0:
		s.add({ 0.0, 0.0, 0.0, 0.5, 0.5, 1.0 });
		s.add({ 0.5, 0.0, 0.0, 1.0, 1.0, 1.0 });
		break;
	case 1:
		s.add({ 0.0, 0.0, 0.0, 0.5, 1.0, 1.0 });
		s.add({ 0.5, 0.0, 0.0, 1.0, 0.5, 1.0 });
		break;
	case 2:
		s.add({ 0.0, 0.0, 0.0, 1.0, 0.5, 0.5 });
		s.add({ 0.0, 0.0, 0.5, 1.0, 1.0, 1.0 });
		break;
	case 3:
		s.add({ 0.0, 0.0, 0.0, 1.0, 1.0, 0.5 });
		s.add({ 0.0, 0.0, 0.5, 1.0, 0.5, 1.0 });
		break;
	}
	return s;
}

// cactus
static AABB cactusAABB(uint8_t) {
	constexpr double I = 0.0625;
	return { I, 0.0, I, 1.0 - I, 1.0, 1.0 - I };
}
static CollisionShape cactusCollider(uint8_t) {
	constexpr double I = 0.0625;
	CollisionShape s;
	s.add({ I, 0.0, I, 1.0 - I, 1.0 - I, 1.0 - I });
	return s;
}

// snow layer
static AABB snowLayerAABB(uint8_t meta) {
	float h = (2.0f * (1 + (meta & 7))) / 16.0f;
	return { 0.0, 0.0, 0.0, 1.0, h, 1.0 };
}
static CollisionShape snowLayerCollider(uint8_t meta) {
	CollisionShape s;
	if ((meta & 7) >= 3)
		s.add({ 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 });
	return s;
}

// ladder
static AABB ladderAABB(uint8_t meta) {
	constexpr double T = 0.125;
	switch (meta) {
	case 2:
		return { 0.0, 0.0, 1.0 - T, 1.0, 1.0, 1.0 };
	case 3:
		return { 0.0, 0.0, 0.0, 1.0, 1.0, T };
	case 4:
		return { 1.0 - T, 0.0, 0.0, 1.0, 1.0, 1.0 };
	case 5:
		return { 0.0, 0.0, 0.0, T, 1.0, 1.0 };
	default:
		return { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
	}
}
static CollisionShape ladderCollider(uint8_t meta) {
	constexpr double T = 0.125;
	CollisionShape s;
	switch (meta) {
	case 2:
		s.add({ 0.0, 0.0, 1.0 - T, 1.0, 1.0, 1.0 });
		break;
	case 3:
		s.add({ 0.0, 0.0, 0.0, 1.0, 1.0, T });
		break;
	case 4:
		s.add({ 1.0 - T, 0.0, 0.0, 1.0, 1.0, 1.0 });
		break;
	case 5:
		s.add({ 0.0, 0.0, 0.0, T, 1.0, 1.0 });
		break;
	}
	return s;
}

// door
// bits 0-1 = facing when closed, bit 2 = open, bit 3 = top half
static int doorState(uint8_t meta) {
	return ((meta & 4) == 0) ? ((meta - 1) & 3) : (meta & 3);
}
static AABB doorAABB(uint8_t meta) {
	constexpr double T = 0.1875;
	switch (doorState(meta)) {
	case 0:
		return { 0.0, 0.0, 0.0, 1.0, 1.0, T };
	case 1:
		return { 1.0 - T, 0.0, 0.0, 1.0, 1.0, 1.0 };
	case 2:
		return { 0.0, 0.0, 1.0 - T, 1.0, 1.0, 1.0 };
	case 3:
		return { 0.0, 0.0, 0.0, T, 1.0, 1.0 };
	default:
		return { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
	}
}
static CollisionShape doorCollider(uint8_t meta) {
	constexpr double T = 0.1875;
	CollisionShape s;
	switch (doorState(meta)) {
	case 0:
		s.add({ 0.0, 0.0, 0.0, 1.0, 1.0, T });
		break;
	case 1:
		s.add({ 1.0 - T, 0.0, 0.0, 1.0, 1.0, 1.0 });
		break;
	case 2:
		s.add({ 0.0, 0.0, 1.0 - T, 1.0, 1.0, 1.0 });
		break;
	case 3:
		s.add({ 0.0, 0.0, 0.0, T, 1.0, 1.0 });
		break;
	}
	return s;
}

// trapdoor
static AABB trapdoorAABB(uint8_t meta) {
	constexpr double T = 0.1875;
	if (!(meta & 4))
		return { 0.0, 0.0, 0.0, 1.0, T, 1.0 };
	switch (meta & 3) {
	case 0:
		return { 0.0, 0.0, 1.0 - T, 1.0, 1.0, 1.0 };
	case 1:
		return { 0.0, 0.0, 0.0, 1.0, 1.0, T };
	case 2:
		return { 1.0 - T, 0.0, 0.0, 1.0, 1.0, 1.0 };
	case 3:
		return { 0.0, 0.0, 0.0, T, 1.0, 1.0 };
	default:
		return { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
	}
}

static CollisionShape farmlandCollider(uint8_t) {
	CollisionShape s;
	s.add({ 0.0, 0.0, 0.0, 1.0, 0.9375, 1.0 });
	return s;
}

static CollisionShape trapdoorCollider(uint8_t meta) {
	constexpr double T = 0.1875;
	CollisionShape s;
	if (!(meta & 4)) {
		s.add({ 0.0, 0.0, 0.0, 1.0, T, 1.0 });
		return s;
	}
	switch (meta & 3) {
	case 0:
		s.add({ 0.0, 0.0, 1.0 - T, 1.0, 1.0, 1.0 });
		break;
	case 1:
		s.add({ 0.0, 0.0, 0.0, 1.0, 1.0, T });
		break;
	case 2:
		s.add({ 1.0 - T, 0.0, 0.0, 1.0, 1.0, 1.0 });
		break;
	case 3:
		s.add({ 0.0, 0.0, 0.0, T, 1.0, 1.0 });
		break;
	}
	return s;
}

// bed
static AABB bedAABB(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.5625, 1.0 };
}
static CollisionShape bedCollider(uint8_t) {
	CollisionShape s;
	s.add({ 0.0, 0.0, 0.0, 1.0, 0.5625, 1.0 });
	return s;
}

// fence
static CollisionShape fenceCollider(uint8_t) {
	CollisionShape s;
	s.add({ 0.0, 0.0, 0.0, 1.0, 1.5, 1.0 });
	return s;
}

// cake
static AABB cakeAABB(uint8_t meta) {
	double x0 = (1 + meta * 2) / 16.0;
	return { x0, 0.0, 0.0625, 1.0 - 0.0625, 0.5 - 0.0625, 1.0 - 0.0625 };
}
static CollisionShape cakeCollider(uint8_t meta) {
	double x0 = (1 + meta * 2) / 16.0;
	CollisionShape s;
	s.add({ x0, 0.0, 0.0625, 1.0 - 0.0625, 0.5 - 0.0625, 1.0 - 0.0625 });
	return s;
}

// repeater
static AABB repeaterAABB(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.125, 1.0 };
}
static CollisionShape emptyCollider(uint8_t) {
	return {};
}

// button
static AABB buttonAABB(uint8_t meta) {
	const int face = meta & 7;
	const bool pressed = (meta & 8) != 0;
	constexpr double lo = 0.375, hi = 0.625, hw = 0.1875;
	const double depth = pressed ? 0.0625 : 0.125;
	switch (face) {
	case 1:
		return { 0.0, lo, 0.5 - hw, depth, hi, 0.5 + hw };
	case 2:
		return { 1.0 - depth, lo, 0.5 - hw, 1.0, hi, 0.5 + hw };
	case 3:
		return { 0.5 - hw, lo, 0.0, 0.5 + hw, hi, depth };
	case 4:
		return { 0.5 - hw, lo, 1.0 - depth, 0.5 + hw, hi, 1.0 };
	default:
		return {};
	}
}

// lever
static AABB leverAABB(uint8_t meta) {
	constexpr double f = 0.1875;
	switch (meta & 7) {
	case 1:
		return { 0.0, 0.2, 0.5 - f, f * 2.0, 0.8, 0.5 + f };
	case 2:
		return { 1.0 - f * 2.0, 0.2, 0.5 - f, 1.0, 0.8, 0.5 + f };
	case 3:
		return { 0.5 - f, 0.2, 0.0, 0.5 + f, 0.8, f * 2.0 };
	case 4:
		return { 0.5 - f, 0.2, 1.0 - f * 2.0, 0.5 + f, 0.8, 1.0 };
	default: {
		constexpr double g = 0.25;
		return { 0.5 - g, 0.0, 0.5 - g, 0.5 + g, 0.6, 0.5 + g };
	}
	}
}

// pressure plate
static AABB pressurePlateAABB(uint8_t meta) {
	constexpr double f = 0.0625;
	return { f, 0.0, f, 1.0 - f, (meta == 1) ? 0.03125 : 0.0625, 1.0 - f };
}

// torch (normal + redstone, same box)
static AABB torchAABB(uint8_t meta) {
	constexpr double f = 0.15;
	switch (meta & 7) {
	case 1:
		return { 0.0, 0.2, 0.5 - f, f * 2.0, 0.8, 0.5 + f };
	case 2:
		return { 1.0 - f * 2.0, 0.2, 0.5 - f, 1.0, 0.8, 0.5 + f };
	case 3:
		return { 0.5 - f, 0.2, 0.0, 0.5 + f, 0.8, f * 2.0 };
	case 4:
		return { 0.5 - f, 0.2, 1.0 - f * 2.0, 0.5 + f, 0.8, 1.0 };
	default: {
		constexpr double g = 0.1;
		return { 0.5 - g, 0.0, 0.5 - g, 0.5 + g, 0.6, 0.5 + g };
	}
	}
}

// rail
static AABB railAABB(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.125, 1.0 };
}

// redstone dust
static AABB redstoneDustAABB(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.0625, 1.0 };
}

// farmland
// Collider is full cube; ray/selection use visual height 0.937
static AABB farmlandAABB(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
}

// crop
static AABB cropAABB(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.25, 1.0 }; // 4/16
}

// sapling / deadbush (f=0.4)
static AABB saplingAABB(uint8_t) {
	constexpr float f = 0.4f;
	return { 0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 2.0f, 0.5f + f };
}

// tall grass
static AABB tallGrassAABB(uint8_t) {
	constexpr float f = 0.4f;
	return { 0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.8f, 0.5f + f };
}

// mushroom (f=0.2)
static AABB mushroomAABB(uint8_t) {
	constexpr float f = 0.2f;
	return { 0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 2.0f, 0.5f + f };
}

// plant / flower (rose, dandelion) (f=0.2, h=f*3)
static AABB plantAABB(uint8_t) {
	constexpr float f = 0.2f;
	return { 0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 3.0f, 0.5f + f };
}

// sugarcane
static AABB sugarcaneAABB(uint8_t) {
	constexpr float f = 0.375f;
	return { 0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 1.0f, 0.5f + f };
}

// Liquids have no collision
static AABB liquidAABB(uint8_t) {
	return { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
}

// piston head
static AABB pistonHeadAABB(uint8_t meta) {
	switch (meta & 7) {
	case 0:
		return { 0.0, 0.0, 0.0, 1.0, 0.25, 1.0 };
	case 1:
		return { 0.0, 0.75, 0.0, 1.0, 1.0, 1.0 };
	case 2:
		return { 0.0, 0.0, 0.0, 1.0, 1.0, 0.25 };
	case 3:
		return { 0.0, 0.0, 0.75, 1.0, 1.0, 1.0 };
	case 4:
		return { 0.0, 0.0, 0.0, 0.25, 1.0, 1.0 };
	case 5:
		return { 0.75, 0.0, 0.0, 1.0, 1.0, 1.0 };
	default:
		return { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
	}
}
static CollisionShape pistonHeadCollider(uint8_t meta) {
	CollisionShape s;
	switch (meta & 7) {
	case 0:
		s.add({ 0.0, 0.0, 0.0, 1.0, 0.25, 1.0 });
		s.add({ 0.375, 0.25, 0.375, 0.625, 1.0, 0.625 });
		break;
	case 1:
		s.add({ 0.0, 0.75, 0.0, 1.0, 1.0, 1.0 });
		s.add({ 0.375, 0.0, 0.375, 0.625, 0.75, 0.625 });
		break;
	case 2:
		s.add({ 0.0, 0.0, 0.0, 1.0, 1.0, 0.25 });
		s.add({ 0.25, 0.375, 0.25, 0.75, 0.625, 1.0 });
		break;
	case 3:
		s.add({ 0.0, 0.0, 0.75, 1.0, 1.0, 1.0 });
		s.add({ 0.25, 0.375, 0.0, 0.75, 0.625, 0.75 });
		break;
	case 4:
		s.add({ 0.0, 0.0, 0.0, 0.25, 1.0, 1.0 });
		s.add({ 0.375, 0.25, 0.25, 0.625, 0.75, 1.0 });
		break;
	case 5:
		s.add({ 0.75, 0.0, 0.0, 1.0, 1.0, 1.0 });
		s.add({ 0.0, 0.375, 0.25, 0.75, 0.625, 0.75 });
		break;
	}
	return s;
}

std::vector<ItemStack> getBlockDrops(BlockType blockId, uint8_t meta, Java::Random& rng) {
	std::vector<ItemStack> drops;

	if (blockId == BLOCK_AIR) {
		return drops;
	}

	// headache: crops drop multiple items of different types (wheat + seeds)
	if (blockId == BLOCK_CROP_WHEAT) {
		if (meta == MAX_CROP_SIZE) {
			drops.push_back(ItemStack{ Items::Id::WHEAT, 1, 0 });
		}

		for (int i = 0; i < 3; i++) {
			if (rng.nextInt(15) <= static_cast<int>(meta)) {
				drops.push_back(ItemStack{ Items::Id::SEEDS_WHEAT, 1, 0 });
			}
		}

		return drops;
	}

	const BlockBehavior& behavior = blockBehaviors[static_cast<uint8_t>(blockId)];
	int count = behavior.quantityDropped ? behavior.quantityDropped(rng) : 1;
	int16_t damage = behavior.damageDropped ? behavior.damageDropped(meta) : 0;

	for (int i = 0; i < count; i++) {
		ItemId id = behavior.idDropped ? behavior.idDropped(meta, rng) : ItemId(blockId);

		if (id > 0) {
			drops.push_back(ItemStack{ id, 1, damage });
		}
	}

	return drops;
}

void registerAll() {
	// Default all behavior slots to full-cube before per-block overrides
	for (int i = 0; i < 256; i++) {
		blockBehaviors[i].getSelectionBox = defaultAABB;
		blockBehaviors[i].getRayBounds = defaultAABB;
		blockBehaviors[i].getCollider = defaultCollider;
	}

	// block properties

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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
	};

	// Sticky Piston Base
	blockProperties[BlockType::BLOCK_PISTON_STICKY] = {
		.material = Material::Piston(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 0.5f,
		.isOpaqueCube = false,
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
	};

	// Wool (Cloth)
	blockProperties[BlockType::BLOCK_WOOL] = {
		.material = Material::Cloth(),
		.stepSound = StepSound::Cloth,
		.lightOpacity = 255,
		.hardness = 0.8f,
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
	};

	// Chest
	blockProperties[BlockType::BLOCK_CHEST] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 2.5f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
	};

	// Furnace (active/lit)
	blockProperties[BlockType::BLOCK_FURNACE_LIT] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightEmission = 13,
		.lightOpacity = 255,
		.hardness = 3.5f,
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
		.enableStats = false,
	};

	// Wooden Door
	blockProperties[BlockType::BLOCK_DOOR_WOOD] = {
		.material = Material::Wood(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 0,
		.hardness = 3.0f,
		.isOpaqueCube = false,
		.isNormalCube = false,
		.renderAsNormalBlock = false,
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
	};

	// Redstone Ore
	blockProperties[BlockType::BLOCK_ORE_REDSTONE_OFF] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightOpacity = 255,
		.hardness = 3.0f,
		.resistance = 5.0f,
		.notifyNeighborsOnMetaChange = false,
	};

	// Redstone Ore (glowing/lit)
	blockProperties[BlockType::BLOCK_ORE_REDSTONE_ON] = {
		.material = Material::Rock(),
		.stepSound = StepSound::Stone,
		.lightEmission = 9,
		.lightOpacity = 255,
		.hardness = 3.0f,
		.resistance = 5.0f,
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
	};

	// Pumpkin
	blockProperties[BlockType::BLOCK_PUMPKIN] = {
		.material = Material::Pumpkin(),
		.stepSound = StepSound::Wood,
		.lightOpacity = 255,
		.hardness = 1.0f,
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
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
		.notifyNeighborsOnMetaChange = false,
		.enableStats = false,
	};

	// block behaviors (non-default shapes)

	// Liquids/zero-size AABBs
	blockBehaviors[BlockType::BLOCK_WATER_FLOWING] = {
		.getSelectionBox = liquidAABB,
		.getRayBounds = liquidAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_WATER_STILL] = {
		.getSelectionBox = liquidAABB,
		.getRayBounds = liquidAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_LAVA_FLOWING] = {
		.getSelectionBox = liquidAABB,
		.getRayBounds = liquidAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_LAVA_STILL] = {
		.getSelectionBox = liquidAABB,
		.getRayBounds = liquidAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_COBWEB] = {
		.getCollider = emptyCollider,
	};

	// Rails
	blockBehaviors[BlockType::BLOCK_RAIL] = {
		.getRayBounds = railAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_RAIL_POWERED] = {
		.getRayBounds = railAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_RAIL_DETECTOR] = {
		.getRayBounds = railAABB,
		.getCollider = emptyCollider,
	};

	// Redstone dust
	blockBehaviors[BlockType::BLOCK_REDSTONE] = {
		.getSelectionBox = redstoneDustAABB,
		.getRayBounds = redstoneDustAABB,
		.getCollider = emptyCollider,
	};

	// Farmland
	blockBehaviors[BlockType::BLOCK_FARMLAND] = {
		.getSelectionBox = farmlandAABB,
		.getRayBounds = farmlandAABB,
		.getCollider = farmlandCollider,
	};

	// Crops
	blockBehaviors[BlockType::BLOCK_CROP_WHEAT] = {
		.getSelectionBox = cropAABB,
		.getRayBounds = cropAABB,
		.getCollider = emptyCollider,
	};

	// Sapling
	blockBehaviors[BlockType::BLOCK_SAPLING] = {
		.getSelectionBox = saplingAABB,
		.getRayBounds = saplingAABB,
		.getCollider = emptyCollider,
	};

	// Tall grass
	blockBehaviors[BlockType::BLOCK_TALLGRASS] = {
		.getSelectionBox = tallGrassAABB,
		.getRayBounds = tallGrassAABB,
		.getCollider = emptyCollider,
	};

	// Mushrooms
	blockBehaviors[BlockType::BLOCK_MUSHROOM_BROWN] = {
		.getSelectionBox = mushroomAABB,
		.getRayBounds = mushroomAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_MUSHROOM_RED] = {
		.getSelectionBox = mushroomAABB,
		.getRayBounds = mushroomAABB,
		.getCollider = emptyCollider,
	};

	// Flowers (rose, dandelion)
	blockBehaviors[BlockType::BLOCK_ROSE] = {
		.getSelectionBox = plantAABB,
		.getRayBounds = plantAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_DANDELION] = {
		.getSelectionBox = plantAABB,
		.getRayBounds = plantAABB,
		.getCollider = emptyCollider,
	};

	// Dead bush
	blockBehaviors[BlockType::BLOCK_DEADBUSH] = {
		.getSelectionBox = saplingAABB, // same f=0.4 box as sapling
		.getRayBounds = saplingAABB,
		.getCollider = emptyCollider,
	};

	// Sugar cane
	blockBehaviors[BlockType::BLOCK_SUGARCANE] = {
		.getSelectionBox = sugarcaneAABB,
		.getRayBounds = sugarcaneAABB,
		.getCollider = emptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_SLAB] = {
		.getSelectionBox = slabAABB,
		.getRayBounds = slabAABB,
		.getCollider = slabCollider,
	};

	blockBehaviors[BlockType::BLOCK_STAIRS_WOOD] = {
		.getCollider = stairCollider,
		// ray/selection stay as defaultAABB (full cube is correct)
	};
	blockBehaviors[BlockType::BLOCK_STAIRS_COBBLESTONE] = {
		.getCollider = stairCollider,
	};

	blockBehaviors[BlockType::BLOCK_CACTUS] = {
		.getSelectionBox = cactusAABB,
		.getRayBounds = cactusAABB,
		.getCollider = cactusCollider,
	};

	blockBehaviors[BlockType::BLOCK_SNOW_LAYER] = {
		.getRayBounds = snowLayerAABB,
		.getCollider = snowLayerCollider,
		// getSelectionBox stays defaultAABB
	};

	blockBehaviors[BlockType::BLOCK_LADDER] = {
		.getSelectionBox = ladderAABB,
		.getRayBounds = ladderAABB,
		.getCollider = ladderCollider,
	};

	blockBehaviors[BlockType::BLOCK_DOOR_WOOD] = {
		.getSelectionBox = doorAABB,
		.getRayBounds = doorAABB,
		.getCollider = doorCollider,
	};
	blockBehaviors[BlockType::BLOCK_DOOR_IRON] = {
		.getSelectionBox = doorAABB,
		.getRayBounds = doorAABB,
		.getCollider = doorCollider,
	};

	blockBehaviors[BlockType::BLOCK_TRAPDOOR] = {
		.getSelectionBox = trapdoorAABB,
		.getRayBounds = trapdoorAABB,
		.getCollider = trapdoorCollider,
	};

	blockBehaviors[BlockType::BLOCK_BED] = {
		.getSelectionBox = bedAABB,
		.getRayBounds = bedAABB,
		.getCollider = bedCollider,
	};

	blockBehaviors[BlockType::BLOCK_FENCE] = {
		.getCollider = fenceCollider,
		// ray/selection stay as defaultAABB (full cube)
	};

	blockBehaviors[BlockType::BLOCK_CAKE] = {
		.getSelectionBox = cakeAABB,
		.getRayBounds = cakeAABB,
		.getCollider = cakeCollider,
	};

	blockBehaviors[BlockType::BLOCK_REDSTONE_REPEATER_OFF] = {
		.getSelectionBox = repeaterAABB,
		.getRayBounds = repeaterAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_REDSTONE_REPEATER_ON] = {
		.getSelectionBox = repeaterAABB,
		.getRayBounds = repeaterAABB,
		.getCollider = emptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_BUTTON_STONE] = {
		.getSelectionBox = buttonAABB,
		.getRayBounds = buttonAABB,
		.getCollider = emptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_LEVER] = {
		.getRayBounds = leverAABB,
		.getCollider = emptyCollider,
		// getSelectionBox stays defaultAABB
	};

	blockBehaviors[BlockType::BLOCK_PRESSURE_PLATE_STONE] = {
		.getSelectionBox = pressurePlateAABB,
		.getRayBounds = pressurePlateAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_PRESSURE_PLATE_WOOD] = {
		.getSelectionBox = pressurePlateAABB,
		.getRayBounds = pressurePlateAABB,
		.getCollider = emptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_TORCH] = {
		.getSelectionBox = torchAABB,
		.getRayBounds = torchAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_REDSTONE_TORCH_OFF] = {
		.getSelectionBox = torchAABB,
		.getRayBounds = torchAABB,
		.getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_REDSTONE_TORCH_ON] = {
		.getSelectionBox = torchAABB,
		.getRayBounds = torchAABB,
		.getCollider = emptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_PISTON_HEAD] = {
		.getSelectionBox = pistonHeadAABB,
		.getRayBounds = pistonHeadAABB,
		.getCollider = pistonHeadCollider,
	};

	// specific behavioral overrides
	blockBehaviors[BLOCK_WATER_FLOWING].velocityToAddToEntity = [](WorldManager& world, Int3 pos,
	                                                               Vec3& pushVector) -> void {
		Vec3 flowVector = getFluidFlowVector(world, pos);
		pushVector = pushVector + flowVector;
	};
	blockBehaviors[BLOCK_WATER_STILL].velocityToAddToEntity = [](WorldManager& world, Int3 pos,
	                                                             Vec3& pushVector) -> void {
		Vec3 flowVector = getFluidFlowVector(world, pos);
		pushVector = pushVector + flowVector;
	};
	blockBehaviors[BLOCK_CACTUS].onEntityCollidedWithBlock = [](WorldManager& world, Int3 pos, Entity& entity) -> void {
		entity.attackEntityFrom(nullptr, 1);
	};
	blockBehaviors[BLOCK_COBWEB].onEntityCollidedWithBlock = [](WorldManager& world, Int3 pos, Entity& entity) -> void {
		entity.inWeb = true;
	};
	blockBehaviors[BLOCK_SOULSAND].onEntityCollidedWithBlock = [](WorldManager& world, Int3 pos,
	                                                              Entity& entity) -> void {
		entity.motionX *= 0.4;
		entity.motionZ *= 0.4;
	};
	blockBehaviors[BLOCK_SUGARCANE].onNeighborBlockChange = [](WorldManager& world, Int3 pos) -> void {
		// Check to see if our placement is still valid
		auto canStay = [&world, &pos]() -> bool {
			auto belowBlock = world.getBlockId({ pos.x, pos.y - 1, pos.z });
			if (belowBlock == BLOCK_SUGARCANE)
				return true;
			if (belowBlock != BLOCK_GRASS && belowBlock != BLOCK_DIRT)
				return false;
			// Check for water
			int d[4] = { -1, 1, 0, 0 };
			for (int i = 0; i < 4; i++) {
				int dx = d[i];
				int dz = d[3 - i];
				auto adjacentBlock = world.getBlockId({ pos.x + dx, pos.y - 1, pos.z + dz });
				if (adjacentBlock == BLOCK_WATER_FLOWING || adjacentBlock == BLOCK_WATER_STILL)
					return true;
			}
			return false;
		};

		if (!canStay())
			BreakAndDropBlock(world, pos);
	};

	// placement overrides
	blockBehaviors[BLOCK_TORCH].onBlockPlaced = [](WorldManager& world, Int3 pos, Entity& placer,
	                                               PacketData::FaceDirection face) -> void {
		auto meta = world.getMetadata(pos);
		if (face == PacketData::FaceDirection::Y_PLUS && (world.isBlockNormalCube({ pos.x, pos.y - 1, pos.z }) || world.getBlockId({ pos.x, pos.y - 1, pos.z }) == BLOCK_FENCE))
			meta = 5;
		if (face == PacketData::FaceDirection::Z_MINUS && world.isBlockNormalCube({ pos.x, pos.y, pos.z + 1 }))
			meta = 4;
		if (face == PacketData::FaceDirection::Z_PLUS && world.isBlockNormalCube({ pos.x, pos.y, pos.z - 1 }))
			meta = 3;
		if (face == PacketData::FaceDirection::X_MINUS && world.isBlockNormalCube({ pos.x + 1, pos.y, pos.z }))
			meta = 2;
		if (face == PacketData::FaceDirection::X_PLUS && world.isBlockNormalCube({ pos.x - 1, pos.y, pos.z }))
			meta = 1;
		world.setMeta(pos, meta);
	};


	// for when the block is interacted with!
	blockBehaviors[BLOCK_DOOR_WOOD].onBlockActivated = [](WorldManager& world, Int3 pos) -> bool {
		auto meta = world.getMetadata(pos);
		if (meta & 8) {
			// We are the top half of the door
			if (world.getBlockId({ pos.x, pos.y - 1, pos.z }) != BLOCK_DOOR_WOOD)
				// Below us is not the bottom of a door! This is bad!
				return false;
			// Recall this function on the bottom of the door
			blockBehaviors[BLOCK_DOOR_WOOD].onBlockActivated(world, { pos.x, pos.y - 1, pos.z });
			return false;
		}
		// We are the top half so lets open
		Int3 top = { pos.x, pos.y + 1, pos.z };
		if (world.getBlockId(top) == BLOCK_DOOR_WOOD && (world.getMetadata(top) & 8)) {
			world.setMeta(top, uint8_t((meta ^ 4) + 8));
		}
		world.setMeta(pos, uint8_t(meta ^ 4)); // XOR bit 2; flips open/closed
		return false;
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
	blockBehaviors[BLOCK_WOOL].damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta;
	};
	blockBehaviors[BLOCK_LOG].damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta;
	};
	blockBehaviors[BLOCK_SAPLING].damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta & 3;
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
	blockBehaviors[BLOCK_GRAVEL].idDropped = [](uint8_t, Java::Random& rng) -> ItemId {
		return rng.nextInt(10) == 0 ? static_cast<ItemId>(Items::Id::FLINT) : static_cast<ItemId>(BLOCK_GRAVEL);
	};

	blockBehaviors[BLOCK_ORE_LAPIS_LAZULI].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::DYE;
	};
	blockBehaviors[BLOCK_ORE_LAPIS_LAZULI].damageDropped = [](uint8_t) -> ItemDamage {
		return 4;
	};
	blockBehaviors[BLOCK_ORE_LAPIS_LAZULI].quantityDropped = [](Java::Random& rng) -> ItemAmount {
		return 4 + rng.nextInt(5);
	};

	blockBehaviors[BLOCK_ORE_REDSTONE_OFF].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE;
	};
	blockBehaviors[BLOCK_ORE_REDSTONE_OFF].quantityDropped = [](Java::Random& rng) -> ItemAmount {
		return 4 + rng.nextInt(2);
	};
	blockBehaviors[BLOCK_ORE_REDSTONE_ON].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE;
	};
	blockBehaviors[BLOCK_ORE_REDSTONE_ON].quantityDropped = [](Java::Random& rng) -> ItemAmount {
		return 4 + rng.nextInt(2);
	};

	blockBehaviors[BLOCK_GLOWSTONE].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::GLOWSTONE_DUST;
	};
	blockBehaviors[BLOCK_GLOWSTONE].quantityDropped = [](Java::Random& rng) -> ItemAmount {
		return 2 + rng.nextInt(3);
	};

	blockBehaviors[BLOCK_LEAVES].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_SAPLING;
	};
	blockBehaviors[BLOCK_LEAVES].damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta & 3;
	};
	blockBehaviors[BLOCK_LEAVES].quantityDropped = [](Java::Random& rng) -> ItemAmount {
		return rng.nextInt(20) == 0 ? 1 : 0;
	};

	blockBehaviors[BLOCK_TALLGRASS].idDropped = [](uint8_t, Java::Random& rng) -> ItemId {
		return rng.nextInt(8) == 0 ? Items::Id::SEEDS_WHEAT : Items::Id::INVALID;
	};

	// --------------- only drop if it's the correct half of the block being broken ---------------
	// TODO: other half of the block should be removed automatically
	blockBehaviors[BLOCK_DOOR_WOOD].idDropped = [](uint8_t meta, Java::Random&) -> ItemId {
		return (meta & 8) != 0 ? Items::Id::INVALID : Items::Id::DOOR_WOOD;
	};

	blockBehaviors[BLOCK_DOOR_IRON].idDropped = [](uint8_t meta, Java::Random&) -> ItemId {
		return (meta & 8) != 0 ? Items::Id::INVALID : Items::Id::DOOR_IRON;
	};

	blockBehaviors[BLOCK_BED].idDropped = [](uint8_t meta, Java::Random&) -> ItemId {
		return (meta & 8) != 0 ? Items::Id::INVALID : Items::Id::BED;
	};

	blockBehaviors[BLOCK_CLAY].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::CLAY;
	};
	blockBehaviors[BLOCK_CLAY].quantityDropped = [](Java::Random&) -> ItemAmount {
		return 4;
	};

	blockBehaviors[BLOCK_SLAB].damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta;
	};

	blockBehaviors[BLOCK_DOUBLE_SLAB].idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_SLAB;
	};
	blockBehaviors[BLOCK_DOUBLE_SLAB].damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta;
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
}

} // namespace Blocks