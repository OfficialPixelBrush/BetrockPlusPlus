/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "block_properties.h"
#include "entities/entity_falling_block.h"
#include "entities/entity_item.h"
#include "enums/items.h"
#include "tile_entities/tile_entity.h"
#include "world/world.h"

namespace Blocks {

// Global table definitions; declared extern in the header
BlockProperties blockProperties[256] = {};
BlockBehavior blockBehaviors[256] = {};

// Behavior helper functions
static bool canFallAt(WorldManager& world, Int3 position) {
	auto block = world.getBlockId(position);
	if (block == BLOCK_AIR)
		return true;
	if (block == BLOCK_FIRE)
		return true;
	auto material = Blocks::blockProperties[block].m_material.m_type;
	if (material == MaterialType::Lava || material == MaterialType::Water)
		return true;
	return false;
}

void BreakAndDropBlock(WorldManager& world, Int3 pos) {
	BlockType blockId = world.getBlockId({ pos.m_x, pos.m_y, pos.m_z });
	uint8_t meta = world.getMetadata({ pos.m_x, pos.m_y, pos.m_z });
	world.setBlock({ pos.m_x, pos.m_y, pos.m_z }, BLOCK_AIR);

	std::vector<ItemStack> drops = Blocks::getBlockDrops(blockId, meta, world.m_rand);

	for (ItemStack drop : drops) {
		Vec3 dropPos = { double(pos.m_x), double(pos.m_y), double(pos.m_z) };
		float offset = 0.7f;
		dropPos.m_x += (world.m_rand.nextFloat() * offset) + (1.0f - offset) * 0.5;
		dropPos.m_y += (world.m_rand.nextFloat() * offset) + (1.0f - offset) * 0.5;
		dropPos.m_z += (world.m_rand.nextFloat() * offset) + (1.0f - offset) * 0.5;
		ItemEntity item(dropPos);
		item.m_itemStack = drop;
		world.m_entityManager.addEntity(std::make_shared<ItemEntity>(item));
	}
	return;
}

Int3 getAdjacentBlockPos(Int3 pos, PacketData::FaceDirection face) {
	switch (face) {
	case PacketData::FaceDirection::Y_MINUS:
		--pos.m_y;
		break;
	case PacketData::FaceDirection::Y_PLUS:
		++pos.m_y;
		break;
	case PacketData::FaceDirection::Z_MINUS:
		--pos.m_z;
		break;
	case PacketData::FaceDirection::Z_PLUS:
		++pos.m_z;
		break;
	case PacketData::FaceDirection::X_MINUS:
		--pos.m_x;
		break;
	case PacketData::FaceDirection::X_PLUS:
		++pos.m_x;
		break;
	default:
		break;
	}
	return pos;
}

bool canSugarcaneSurviveAt(WorldManager& world, Int3 pos) {
	auto belowBlock = world.getBlockId({ pos.m_x, pos.m_y - 1, pos.m_z });
	if (belowBlock == BLOCK_SUGARCANE)
		return true;
	if (belowBlock != BLOCK_GRASS && belowBlock != BLOCK_DIRT)
		return false;
	// Check for water
	int d[4] = { -1, 1, 0, 0 };
	for (int i = 0; i < 4; i++) {
		int dx = d[i];
		int dz = d[3 - i];
		auto adjacentBlock = world.getBlockId({ pos.m_x + dx, pos.m_y - 1, pos.m_z + dz });
		if (adjacentBlock == BLOCK_WATER_FLOWING || adjacentBlock == BLOCK_WATER_STILL)
			return true;
	}
	return false;
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
		int dx = pos.m_x + ndx[i];
		int dz = pos.m_z + ndz[i];
		int neighborFlowContribution = getEffectiveFlowDecay(world, { dx, pos.m_y, dz }, waterMaterial);
		int flowDifference = 0;
		// Our neighbor block didn't have the same material
		if (neighborFlowContribution < 0) {
			if (!world.getMaterial({ dx, pos.m_y, dz }).m_isSolid) {
				// Check the block below us to see if its water, if it is, STRONGLY pull down
				int belowFlowContribution = getEffectiveFlowDecay(world, { dx, pos.m_y - 1, dz }, waterMaterial);
				if (belowFlowContribution >= 0) {
					flowDifference = belowFlowContribution - (myFlowContribution - 8);
					flowVector.m_x += double((dx - pos.m_x) * flowDifference);
					flowVector.m_z += double((dz - pos.m_z) * flowDifference);
				}
			}
		} else {
			flowDifference = neighborFlowContribution - myFlowContribution;
			flowVector.m_x += double((dx - pos.m_x) * flowDifference);
			flowVector.m_z += double((dz - pos.m_z) * flowDifference);
		}
	}

	auto isFluidWall = [&](Int3 checkPos) {
		Material neighborMaterial = world.getMaterial(checkPos);
		if (neighborMaterial == waterMaterial)
			return false;
		if (neighborMaterial == Material::Ice())
			return false;
		return neighborMaterial.m_isSolid;
	};

	// If we're a falling fluid segment, check whether we're clinging to a wall
	if (world.getMetadata(pos) >= 8) {
		bool nearWall = false;

		if (!nearWall && isFluidWall({ pos.m_x, pos.m_y, pos.m_z - 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.m_x, pos.m_y, pos.m_z + 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.m_x - 1, pos.m_y, pos.m_z }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.m_x + 1, pos.m_y, pos.m_z }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.m_x, pos.m_y + 1, pos.m_z - 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.m_x, pos.m_y + 1, pos.m_z + 1 }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.m_x - 1, pos.m_y + 1, pos.m_z }))
			nearWall = true;
		if (!nearWall && isFluidWall({ pos.m_x + 1, pos.m_y + 1, pos.m_z }))
			nearWall = true;

		if (nearWall) {
			// Normalize what we have so far, then let the huge -6 dominate the normalization after this
			double lenSq = flowVector.m_x * flowVector.m_x + flowVector.m_y * flowVector.m_y + flowVector.m_z * flowVector.m_z;
			if (lenSq > 0.0) {
				double invLen = 1.0 / std::sqrt(lenSq);
				flowVector.m_x *= invLen;
				flowVector.m_y *= invLen;
				flowVector.m_z *= invLen;
			}
			flowVector.m_y += -6.0;
		}
	}

	// Final normalize
	double lenSq = flowVector.m_x * flowVector.m_x + flowVector.m_y * flowVector.m_y + flowVector.m_z * flowVector.m_z;
	if (lenSq > 0.0) {
		double invLen = 1.0 / std::sqrt(lenSq);
		flowVector.m_x *= invLen;
		flowVector.m_y *= invLen;
		flowVector.m_z *= invLen;
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
	int count = behavior.m_quantityDropped ? behavior.m_quantityDropped(rng) : 1;
	int16_t damage = behavior.m_damageDropped ? behavior.m_damageDropped(meta) : 0;

	for (int i = 0; i < count; i++) {
		ItemId id = behavior.m_idDropped ? behavior.m_idDropped(meta, rng) : ItemId(blockId);

		if (id > 0) {
			drops.push_back(ItemStack{ id, 1, damage });
		}
	}

	return drops;
}

void registerAll() {
	// Default all behavior slots to full-cube before per-block overrides
	for (int i = 0; i < 256; i++) {
		blockBehaviors[i].m_getSelectionBox = defaultAABB;
		blockBehaviors[i].m_getRayBounds = defaultAABB;
		blockBehaviors[i].m_getCollider = defaultCollider;
	}

	// block properties

	// Air
	blockProperties[BlockType::BLOCK_AIR] = {
		.m_material = Material::Air(),
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_resistance = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_canBlockGrass = false,
		.m_enableStats = false,
	};

	// Stone
	blockProperties[BlockType::BLOCK_STONE] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 1.5f,
		.m_resistance = 10.0f,
	};

	// Grass
	blockProperties[BlockType::BLOCK_GRASS] = {
		.m_material = Material::Grass(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 255,
		.m_hardness = 0.6f,
		.m_ticksOnLoad = true,
	};

	// Dirt
	blockProperties[BlockType::BLOCK_DIRT] = {
		.m_material = Material::Ground(),
		.m_stepSound = StepSound::Gravel,
		.m_lightOpacity = 255,
		.m_hardness = 0.5f,
	};

	// Cobblestone
	blockProperties[BlockType::BLOCK_COBBLESTONE] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 2.0f,
		.m_resistance = 10.0f,
	};

	// Planks (Oak Wood)
	blockProperties[BlockType::BLOCK_PLANKS] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 255,
		.m_hardness = 2.0f,
		.m_resistance = 5.0f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Sapling
	blockProperties[BlockType::BLOCK_SAPLING] = {
		.m_material = Material::Plants(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_ticksOnLoad = true,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Bedrock
	blockProperties[BlockType::BLOCK_BEDROCK] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_hardness = -1.0f, // unbreakable
		.m_resistance = 6000000.0f,
		.m_enableStats = false,
	};

	// Water (flowing)
	blockProperties[BlockType::BLOCK_WATER_FLOWING] = {
		.m_material = Material::Water(),
		.m_lightOpacity = 3,
		.m_hardness = 100.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Water (still/stationary)
	blockProperties[BlockType::BLOCK_WATER_STILL] = {
		.m_material = Material::Water(),
		.m_lightOpacity = 3,
		.m_hardness = 100.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Lava (flowing)
	blockProperties[BlockType::BLOCK_LAVA_FLOWING] = {
		.m_material = Material::Lava(),
		.m_lightEmission = 15, // setLightValue(1.0f) -> 15*1.0 = 15
		.m_lightOpacity = 255,
		.m_hardness = 0.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Lava (still/stationary)
	blockProperties[BlockType::BLOCK_LAVA_STILL] = {
		.m_material = Material::Lava(),
		.m_lightEmission = 15,
		.m_lightOpacity = 255,
		.m_hardness = 100.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Sand
	blockProperties[BlockType::BLOCK_SAND] = {
		.m_material = Material::Sand(),
		.m_stepSound = StepSound::Sand,
		.m_lightOpacity = 255,
		.m_hardness = 0.5f,
	};

	// Gravel
	blockProperties[BlockType::BLOCK_GRAVEL] = {
		.m_material = Material::Sand(),
		.m_stepSound = StepSound::Gravel,
		.m_lightOpacity = 255,
		.m_hardness = 0.6f,
	};

	// Gold Ore
	blockProperties[BlockType::BLOCK_ORE_GOLD] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 3.0f,
		.m_resistance = 5.0f,
	};

	// Iron Ore
	blockProperties[BlockType::BLOCK_ORE_IRON] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 3.0f,
		.m_resistance = 5.0f,
	};

	// Coal Ore
	blockProperties[BlockType::BLOCK_ORE_COAL] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 3.0f,
		.m_resistance = 5.0f,
	};

	// Wood Log
	blockProperties[BlockType::BLOCK_LOG] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 255,
		.m_hardness = 2.0f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Leaves
	blockProperties[BlockType::BLOCK_LEAVES] = {
		.m_material = Material::Leaves(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 1,
		.m_hardness = 0.2f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_ticksOnLoad = true,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Sponge
	blockProperties[BlockType::BLOCK_SPONGE] = {
		.m_material = Material::Sponge(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 255,
		.m_hardness = 0.6f,
	};

	// Glass
	blockProperties[BlockType::BLOCK_GLASS] = {
		.m_material = Material::Glass(),
		.m_stepSound = StepSound::Glass,
		.m_lightOpacity = 0,
		.m_hardness = 0.3f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
	};

	// Lapis Lazuli Ore
	blockProperties[BlockType::BLOCK_ORE_LAPIS_LAZULI] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 3.0f,
		.m_resistance = 5.0f,
	};

	// Lapis Lazuli Block
	blockProperties[BlockType::BLOCK_LAPIS_LAZULI] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 3.0f,
		.m_resistance = 5.0f,
	};

	// Dispenser
	blockProperties[BlockType::BLOCK_DISPENSER] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 3.5f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Sandstone
	blockProperties[BlockType::BLOCK_SANDSTONE] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 0.8f,
	};

	// Note Block
	blockProperties[BlockType::BLOCK_NOTEBLOCK] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 255,
		.m_hardness = 0.8f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Bed
	blockProperties[BlockType::BLOCK_BED] = {
		.m_material = Material::Cloth(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 0.2f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Powered Rail (Golden Rail)
	blockProperties[BlockType::BLOCK_RAIL_POWERED] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 0,
		.m_hardness = 0.7f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Detector Rail
	blockProperties[BlockType::BLOCK_RAIL_DETECTOR] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 0,
		.m_hardness = 0.7f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Sticky Piston Base
	blockProperties[BlockType::BLOCK_PISTON_STICKY] = {
		.m_material = Material::Piston(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 0.5f,
		.m_isOpaqueCube = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Cobweb
	blockProperties[BlockType::BLOCK_COBWEB] = {
		.m_material = Material::Web(),
		.m_stepSound = StepSound::Cloth,
		.m_lightOpacity = 1,
		.m_hardness = 4.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
	};

	// Tall Grass
	blockProperties[BlockType::BLOCK_TALLGRASS] = {
		.m_material = Material::Plants(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
	};

	// Dead Bush
	blockProperties[BlockType::BLOCK_DEADBUSH] = {
		.m_material = Material::Plants(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
	};

	// Piston Base
	blockProperties[BlockType::BLOCK_PISTON] = {
		.m_material = Material::Piston(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 0.5f,
		.m_isOpaqueCube = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Piston Extension (head)
	blockProperties[BlockType::BLOCK_PISTON_HEAD] = {
		.m_material = Material::Piston(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 0,
		.m_hardness = 0.5f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Wool (Cloth)
	blockProperties[BlockType::BLOCK_WOOL] = {
		.m_material = Material::Cloth(),
		.m_stepSound = StepSound::Cloth,
		.m_lightOpacity = 255,
		.m_hardness = 0.8f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Piston Moving (tile entity placeholder)
	blockProperties[BlockType::BLOCK_PISTON_MOVING] = {
		.m_material = Material::Piston(),
		.m_lightOpacity = 0,
		.m_hardness = -1.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_enableStats = false,
	};

	// Dandelion (Yellow Flower)
	blockProperties[BlockType::BLOCK_DANDELION] = {
		.m_material = Material::Plants(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
	};

	// Rose (Red Flower)
	blockProperties[BlockType::BLOCK_ROSE] = {
		.m_material = Material::Plants(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
	};

	// Brown Mushroom
	blockProperties[BlockType::BLOCK_MUSHROOM_BROWN] = {
		.m_material = Material::Plants(),
		.m_stepSound = StepSound::Grass,
		.m_lightEmission = 1,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
	};

	// Red Mushroom
	blockProperties[BlockType::BLOCK_MUSHROOM_RED] = {
		.m_material = Material::Plants(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
	};

	// Gold Block
	blockProperties[BlockType::BLOCK_GOLD] = {
		.m_material = Material::Iron(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 3.0f,
		.m_resistance = 10.0f,
	};

	// Iron Block
	blockProperties[BlockType::BLOCK_IRON] = {
		.m_material = Material::Iron(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 5.0f,
		.m_resistance = 10.0f,
	};

	// Double Stone Slab
	blockProperties[BlockType::BLOCK_DOUBLE_SLAB] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 2.0f,
		.m_resistance = 10.0f,
	};

	// Stone Slab (single)
	blockProperties[BlockType::BLOCK_SLAB] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 2.0f,
		.m_resistance = 10.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
	};

	// Bricks
	blockProperties[BlockType::BLOCK_BRICKS] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 2.0f,
		.m_resistance = 10.0f,
	};

	// TNT
	blockProperties[BlockType::BLOCK_TNT] = {
		.m_material = Material::TNT(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 255,
		.m_hardness = 0.0f,
	};

	// Bookshelf
	blockProperties[BlockType::BLOCK_BOOKSHELF] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 255,
		.m_hardness = 1.5f,
	};

	// Mossy Cobblestone
	blockProperties[BlockType::BLOCK_COBBLESTONE_MOSSY] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 2.0f,
		.m_resistance = 10.0f,
	};

	// Obsidian
	blockProperties[BlockType::BLOCK_OBSIDIAN] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 10.0f,
		.m_resistance = 2000.0f,
	};

	// Torch
	blockProperties[BlockType::BLOCK_TORCH] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Wood,
		.m_lightEmission = 14,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Fire
	blockProperties[BlockType::BLOCK_FIRE] = {
		.m_material = Material::Fire(),
		.m_lightEmission = 15,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_canBlockGrass = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Monster Spawner
	blockProperties[BlockType::BLOCK_MOB_SPAWNER] = {
		.m_material = Material::Iron(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 0,
		.m_hardness = 5.0f,
		.m_enableStats = false,
	};

	// Oak Wood Stairs
	blockProperties[BlockType::BLOCK_STAIRS_WOOD] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 255,
		.m_hardness = 2.0f,
		.m_resistance = 5.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Chest
	blockProperties[BlockType::BLOCK_CHEST] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 2.5f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Redstone Wire
	blockProperties[BlockType::BLOCK_REDSTONE] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Diamond Ore
	blockProperties[BlockType::BLOCK_ORE_DIAMOND] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 3.0f,
		.m_resistance = 5.0f,
	};

	// Diamond Block
	blockProperties[BlockType::BLOCK_DIAMOND] = {
		.m_material = Material::Iron(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 5.0f,
		.m_resistance = 10.0f,
	};

	// Crafting Table (Workbench)
	blockProperties[BlockType::BLOCK_CRAFTING_TABLE] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 255,
		.m_hardness = 2.5f,
	};

	// Crops / Wheat
	blockProperties[BlockType::BLOCK_CROP_WHEAT] = {
		.m_material = Material::Plants(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_ticksOnLoad = true,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Farmland (Tilled Field)
	blockProperties[BlockType::BLOCK_FARMLAND] = {
		.m_material = Material::Ground(),
		.m_stepSound = StepSound::Gravel,
		.m_lightOpacity = 255,
		.m_hardness = 0.6f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
	};

	// Furnace (idle)
	blockProperties[BlockType::BLOCK_FURNACE] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 3.5f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Furnace (active/lit)
	blockProperties[BlockType::BLOCK_FURNACE_LIT] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightEmission = 13,
		.m_lightOpacity = 255,
		.m_hardness = 3.5f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Sign (standing)
	blockProperties[BlockType::BLOCK_SIGN] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 1.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Wooden Door
	blockProperties[BlockType::BLOCK_DOOR_WOOD] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 3.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Ladder
	blockProperties[BlockType::BLOCK_LADDER] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 0.4f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Rail (normal)
	blockProperties[BlockType::BLOCK_RAIL] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 0,
		.m_hardness = 0.7f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Cobblestone Stairs
	blockProperties[BlockType::BLOCK_STAIRS_COBBLESTONE] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 2.0f,
		.m_resistance = 10.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Wall Sign
	blockProperties[BlockType::BLOCK_SIGN_WALL] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 1.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Lever
	blockProperties[BlockType::BLOCK_LEVER] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 0.5f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Stone Pressure Plate
	blockProperties[BlockType::BLOCK_PRESSURE_PLATE_STONE] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 0,
		.m_hardness = 0.5f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Iron Door
	blockProperties[BlockType::BLOCK_DOOR_IRON] = {
		.m_material = Material::Iron(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 0,
		.m_hardness = 5.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Wooden Pressure Plate
	blockProperties[BlockType::BLOCK_PRESSURE_PLATE_WOOD] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 0.5f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Redstone Ore
	blockProperties[BlockType::BLOCK_ORE_REDSTONE_OFF] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 3.0f,
		.m_resistance = 5.0f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Redstone Ore (glowing/lit)
	blockProperties[BlockType::BLOCK_ORE_REDSTONE_ON] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightEmission = 9,
		.m_lightOpacity = 255,
		.m_hardness = 3.0f,
		.m_resistance = 5.0f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Redstone Torch (off)
	blockProperties[BlockType::BLOCK_REDSTONE_TORCH_OFF] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Redstone Torch (on)
	blockProperties[BlockType::BLOCK_REDSTONE_TORCH_ON] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Wood,
		.m_lightEmission = 7,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Stone Button
	blockProperties[BlockType::BLOCK_BUTTON_STONE] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 0,
		.m_hardness = 0.5f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Snow (layer)
	blockProperties[BlockType::BLOCK_SNOW_LAYER] = {
		.m_material = Material::SnowLayer(),
		.m_stepSound = StepSound::Cloth,
		.m_lightOpacity = 0,
		.m_hardness = 0.1f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_canBlockGrass = false,
	};

	// Ice
	blockProperties[BlockType::BLOCK_ICE] = {
		.m_material = Material::Ice(),
		.m_stepSound = StepSound::Glass,
		.m_lightOpacity = 3,
		.m_hardness = 0.5f,
		.m_slipperiness = 0.98f,
	};

	// Snow Block
	blockProperties[BlockType::BLOCK_SNOW] = {
		.m_material = Material::SnowBlock(),
		.m_stepSound = StepSound::Cloth,
		.m_lightOpacity = 255,
		.m_hardness = 0.2f,
	};

	// Cactus
	blockProperties[BlockType::BLOCK_CACTUS] = {
		.m_material = Material::Cactus(),
		.m_stepSound = StepSound::Cloth,
		.m_lightOpacity = 0,
		.m_hardness = 0.4f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_ticksOnLoad = true,
	};

	// Clay Block
	blockProperties[BlockType::BLOCK_CLAY] = {
		.m_material = Material::Clay(),
		.m_stepSound = StepSound::Gravel,
		.m_lightOpacity = 255,
		.m_hardness = 0.6f,
	};

	// Sugar Cane (Reed)
	blockProperties[BlockType::BLOCK_SUGARCANE] = {
		.m_material = Material::Plants(),
		.m_stepSound = StepSound::Grass,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_ticksOnLoad = true,
		.m_enableStats = false,
	};

	// Jukebox
	blockProperties[BlockType::BLOCK_JUKEBOX] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 2.0f,
		.m_resistance = 10.0f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Fence
	blockProperties[BlockType::BLOCK_FENCE] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 2.0f,
		.m_resistance = 5.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Pumpkin
	blockProperties[BlockType::BLOCK_PUMPKIN] = {
		.m_material = Material::Pumpkin(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 255,
		.m_hardness = 1.0f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Netherrack
	blockProperties[BlockType::BLOCK_NETHERRACK] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Stone,
		.m_lightOpacity = 255,
		.m_hardness = 0.4f,
	};

	// Soul Sand
	blockProperties[BlockType::BLOCK_SOULSAND] = {
		.m_material = Material::Sand(),
		.m_stepSound = StepSound::Sand,
		.m_lightOpacity = 255,
		.m_hardness = 0.5f,
	};

	// Glowstone
	blockProperties[BlockType::BLOCK_GLOWSTONE] = {
		.m_material = Material::Rock(),
		.m_stepSound = StepSound::Glass,
		.m_lightEmission = 15,
		.m_lightOpacity = 255,
		.m_hardness = 0.3f,
	};

	// Nether Portal
	blockProperties[BlockType::BLOCK_NETHER_PORTAL] = {
		.m_material = Material::Portal(),
		.m_stepSound = StepSound::Glass,
		.m_lightEmission = 11,
		.m_lightOpacity = 0,
		.m_hardness = -1.0f,
		.m_isCollidable = false,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
	};

	// Jack-o-Lantern (Lit Pumpkin)
	blockProperties[BlockType::BLOCK_PUMPKIN_LIT] = {
		.m_material = Material::Pumpkin(),
		.m_stepSound = StepSound::Wood,
		.m_lightEmission = 15,
		.m_lightOpacity = 255,
		.m_hardness = 1.0f,
		.m_notifyNeighborsOnMetaChange = false,
	};

	// Cake
	blockProperties[BlockType::BLOCK_CAKE] = {
		.m_material = Material::Cake(),
		.m_stepSound = StepSound::Cloth,
		.m_lightOpacity = 0,
		.m_hardness = 0.5f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Redstone Repeater (off)
	blockProperties[BlockType::BLOCK_REDSTONE_REPEATER_OFF] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Redstone Repeater (on)
	blockProperties[BlockType::BLOCK_REDSTONE_REPEATER_ON] = {
		.m_material = Material::Circuits(),
		.m_stepSound = StepSound::Wood,
		.m_lightEmission = 9,
		.m_lightOpacity = 0,
		.m_hardness = 0.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// Trapdoor
	blockProperties[BlockType::BLOCK_TRAPDOOR] = {
		.m_material = Material::Wood(),
		.m_stepSound = StepSound::Wood,
		.m_lightOpacity = 0,
		.m_hardness = 3.0f,
		.m_isOpaqueCube = false,
		.m_isNormalCube = false,
		.m_renderAsNormalBlock = false,
		.m_notifyNeighborsOnMetaChange = false,
		.m_enableStats = false,
	};

	// block behaviors (non-default shapes)

	// Liquids/zero-size AABBs
	blockBehaviors[BlockType::BLOCK_WATER_FLOWING] = {
		.m_getSelectionBox = liquidAABB,
		.m_getRayBounds = liquidAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_WATER_STILL] = {
		.m_getSelectionBox = liquidAABB,
		.m_getRayBounds = liquidAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_LAVA_FLOWING] = {
		.m_getSelectionBox = liquidAABB,
		.m_getRayBounds = liquidAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_LAVA_STILL] = {
		.m_getSelectionBox = liquidAABB,
		.m_getRayBounds = liquidAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_COBWEB] = {
		.m_getCollider = emptyCollider,
	};

	// Rails
	blockBehaviors[BlockType::BLOCK_RAIL] = {
		.m_getRayBounds = railAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_RAIL_POWERED] = {
		.m_getRayBounds = railAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_RAIL_DETECTOR] = {
		.m_getRayBounds = railAABB,
		.m_getCollider = emptyCollider,
	};

	// Redstone dust
	blockBehaviors[BlockType::BLOCK_REDSTONE] = {
		.m_getSelectionBox = redstoneDustAABB,
		.m_getRayBounds = redstoneDustAABB,
		.m_getCollider = emptyCollider,
	};

	// Farmland
	blockBehaviors[BlockType::BLOCK_FARMLAND] = {
		.m_getSelectionBox = farmlandAABB,
		.m_getRayBounds = farmlandAABB,
		.m_getCollider = farmlandCollider,
	};

	// Crops
	blockBehaviors[BlockType::BLOCK_CROP_WHEAT] = {
		.m_getSelectionBox = cropAABB,
		.m_getRayBounds = cropAABB,
		.m_getCollider = emptyCollider,
	};

	// Sapling
	blockBehaviors[BlockType::BLOCK_SAPLING] = {
		.m_getSelectionBox = saplingAABB,
		.m_getRayBounds = saplingAABB,
		.m_getCollider = emptyCollider,
	};

	// Tall grass
	blockBehaviors[BlockType::BLOCK_TALLGRASS] = {
		.m_getSelectionBox = tallGrassAABB,
		.m_getRayBounds = tallGrassAABB,
		.m_getCollider = emptyCollider,
	};

	// Mushrooms
	blockBehaviors[BlockType::BLOCK_MUSHROOM_BROWN] = {
		.m_getSelectionBox = mushroomAABB,
		.m_getRayBounds = mushroomAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_MUSHROOM_RED] = {
		.m_getSelectionBox = mushroomAABB,
		.m_getRayBounds = mushroomAABB,
		.m_getCollider = emptyCollider,
	};

	// Flowers (rose, dandelion)
	blockBehaviors[BlockType::BLOCK_ROSE] = {
		.m_getSelectionBox = plantAABB,
		.m_getRayBounds = plantAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_DANDELION] = {
		.m_getSelectionBox = plantAABB,
		.m_getRayBounds = plantAABB,
		.m_getCollider = emptyCollider,
	};

	// Dead bush
	blockBehaviors[BlockType::BLOCK_DEADBUSH] = {
		.m_getSelectionBox = saplingAABB, // same f=0.4 box as sapling
		.m_getRayBounds = saplingAABB,
		.m_getCollider = emptyCollider,
	};

	// Sugar cane
	blockBehaviors[BlockType::BLOCK_SUGARCANE] = {
		.m_getSelectionBox = sugarcaneAABB,
		.m_getRayBounds = sugarcaneAABB,
		.m_getCollider = emptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_SLAB] = {
		.m_getSelectionBox = slabAABB,
		.m_getRayBounds = slabAABB,
		.m_getCollider = slabCollider,
	};

	blockBehaviors[BlockType::BLOCK_STAIRS_WOOD] = {
		.m_getCollider = stairCollider,
		// ray/selection stay as defaultAABB (full cube is correct)
	};
	blockBehaviors[BlockType::BLOCK_STAIRS_COBBLESTONE] = {
		.m_getCollider = stairCollider,
	};

	blockBehaviors[BlockType::BLOCK_CACTUS] = {
		.m_getSelectionBox = cactusAABB,
		.m_getRayBounds = cactusAABB,
		.m_getCollider = cactusCollider,
	};

	blockBehaviors[BlockType::BLOCK_SNOW_LAYER] = {
		.m_getRayBounds = snowLayerAABB,
		.m_getCollider = snowLayerCollider,
		// getSelectionBox stays defaultAABB
	};

	blockBehaviors[BlockType::BLOCK_LADDER] = {
		.m_getSelectionBox = ladderAABB,
		.m_getRayBounds = ladderAABB,
		.m_getCollider = ladderCollider,
	};

	blockBehaviors[BlockType::BLOCK_DOOR_WOOD] = {
		.m_getSelectionBox = doorAABB,
		.m_getRayBounds = doorAABB,
		.m_getCollider = doorCollider,
	};
	blockBehaviors[BlockType::BLOCK_DOOR_IRON] = {
		.m_getSelectionBox = doorAABB,
		.m_getRayBounds = doorAABB,
		.m_getCollider = doorCollider,
	};

	blockBehaviors[BlockType::BLOCK_TRAPDOOR] = {
		.m_getSelectionBox = trapdoorAABB,
		.m_getRayBounds = trapdoorAABB,
		.m_getCollider = trapdoorCollider,
	};

	blockBehaviors[BlockType::BLOCK_BED] = {
		.m_getSelectionBox = bedAABB,
		.m_getRayBounds = bedAABB,
		.m_getCollider = bedCollider,
	};

	blockBehaviors[BlockType::BLOCK_FENCE] = {
		.m_getCollider = fenceCollider,
		// ray/selection stay as defaultAABB (full cube)
	};

	blockBehaviors[BlockType::BLOCK_CAKE] = {
		.m_getSelectionBox = cakeAABB,
		.m_getRayBounds = cakeAABB,
		.m_getCollider = cakeCollider,
	};

	blockBehaviors[BlockType::BLOCK_REDSTONE_REPEATER_OFF] = {
		.m_getSelectionBox = repeaterAABB,
		.m_getRayBounds = repeaterAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_REDSTONE_REPEATER_ON] = {
		.m_getSelectionBox = repeaterAABB,
		.m_getRayBounds = repeaterAABB,
		.m_getCollider = emptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_BUTTON_STONE] = {
		.m_getSelectionBox = buttonAABB,
		.m_getRayBounds = buttonAABB,
		.m_getCollider = emptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_LEVER] = {
		.m_getRayBounds = leverAABB,
		.m_getCollider = emptyCollider,
		// getSelectionBox stays defaultAABB
	};

	blockBehaviors[BlockType::BLOCK_PRESSURE_PLATE_STONE] = {
		.m_getSelectionBox = pressurePlateAABB,
		.m_getRayBounds = pressurePlateAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_PRESSURE_PLATE_WOOD] = {
		.m_getSelectionBox = pressurePlateAABB,
		.m_getRayBounds = pressurePlateAABB,
		.m_getCollider = emptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_TORCH] = {
		.m_getSelectionBox = torchAABB,
		.m_getRayBounds = torchAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_REDSTONE_TORCH_OFF] = {
		.m_getSelectionBox = torchAABB,
		.m_getRayBounds = torchAABB,
		.m_getCollider = emptyCollider,
	};
	blockBehaviors[BlockType::BLOCK_REDSTONE_TORCH_ON] = {
		.m_getSelectionBox = torchAABB,
		.m_getRayBounds = torchAABB,
		.m_getCollider = emptyCollider,
	};

	blockBehaviors[BlockType::BLOCK_PISTON_HEAD] = {
		.m_getSelectionBox = pistonHeadAABB,
		.m_getRayBounds = pistonHeadAABB,
		.m_getCollider = pistonHeadCollider,
	};

	// specific behavioral overrides
	blockBehaviors[BLOCK_WATER_FLOWING].m_velocityToAddToEntity = [](WorldManager& world, Int3 pos,
	                                                               Vec3& pushVector) -> void {
		Vec3 flowVector = getFluidFlowVector(world, pos);
		pushVector = pushVector + flowVector;
	};
	blockBehaviors[BLOCK_WATER_STILL].m_velocityToAddToEntity = [](WorldManager& world, Int3 pos,
	                                                             Vec3& pushVector) -> void {
		Vec3 flowVector = getFluidFlowVector(world, pos);
		pushVector = pushVector + flowVector;
	};
	blockBehaviors[BLOCK_CACTUS].m_onEntityCollidedWithBlock = [](WorldManager& world, Int3 pos, Entity& entity) -> void {
		entity.attackEntityFrom(nullptr, 1);
	};
	blockBehaviors[BLOCK_COBWEB].m_onEntityCollidedWithBlock = [](WorldManager& world, Int3 pos, Entity& entity) -> void {
		entity.m_inWeb = true;
	};
	blockBehaviors[BLOCK_SOULSAND].m_onEntityCollidedWithBlock = [](WorldManager& world, Int3 pos,
	                                                              Entity& entity) -> void {
		entity.m_velocity.m_x *= 0.4;
		entity.m_velocity.m_z *= 0.4;
	};
	blockBehaviors[BLOCK_SUGARCANE].m_onNeighborBlockChange = [](WorldManager& world, Int3 pos) -> void {
		// Check to see if our placement is still valid
		if (!canSugarcaneSurviveAt(world, pos))
			BreakAndDropBlock(world, pos);
	};

	// placement overrides
	blockBehaviors[BLOCK_TORCH].m_onBlockPlaced = [](WorldManager& world, Int3 pos, Entity& placer,
	                                               PacketData::FaceDirection face) -> void {
		auto meta = world.getMetadata(pos);
		if (face == PacketData::FaceDirection::Y_PLUS && (world.isBlockNormalCube({ pos.m_x, pos.m_y - 1, pos.m_z }) ||
		                                                  world.getBlockId({ pos.m_x, pos.m_y - 1, pos.m_z }) == BLOCK_FENCE))
			meta = 5;
		if (face == PacketData::FaceDirection::Z_MINUS && world.isBlockNormalCube({ pos.m_x, pos.m_y, pos.m_z + 1 }))
			meta = 4;
		if (face == PacketData::FaceDirection::Z_PLUS && world.isBlockNormalCube({ pos.m_x, pos.m_y, pos.m_z - 1 }))
			meta = 3;
		if (face == PacketData::FaceDirection::X_MINUS && world.isBlockNormalCube({ pos.m_x + 1, pos.m_y, pos.m_z }))
			meta = 2;
		if (face == PacketData::FaceDirection::X_PLUS && world.isBlockNormalCube({ pos.m_x - 1, pos.m_y, pos.m_z }))
			meta = 1;
		world.setMeta(pos, meta);
	};

	// for when the block is interacted with!
	blockBehaviors[BLOCK_DOOR_WOOD].m_onBlockActivated = [](WorldManager& world, Int3 pos) -> bool {
		auto meta = world.getMetadata(pos);
		if (meta & 8) {
			// We are the top half of the door
			if (world.getBlockId({ pos.m_x, pos.m_y - 1, pos.m_z }) != BLOCK_DOOR_WOOD)
				// Below us is not the bottom of a door! This is bad!
				return false;
			// Recall this function on the bottom of the door
			blockBehaviors[BLOCK_DOOR_WOOD].m_onBlockActivated(world, { pos.m_x, pos.m_y - 1, pos.m_z });
			return false;
		}
		// We are the top half so lets open
		Int3 top = { pos.m_x, pos.m_y + 1, pos.m_z };
		if (world.getBlockId(top) == BLOCK_DOOR_WOOD && (world.getMetadata(top) & 8)) {
			world.setMeta(top, uint8_t((meta ^ 4) + 8));
		}
		world.setMeta(pos, uint8_t(meta ^ 4)); // XOR bit 2; flips open/closed
		return false;
	};

	// Falling blocks!
	blockBehaviors[BLOCK_GRAVEL].m_onNeighborBlockChange = [](WorldManager& world, Int3 pos) -> void {
		// Schedule a check to see if we can fall
		world.m_tickScheduler.scheduleUpdateTick(pos, BLOCK_GRAVEL, 3);
	};
	blockBehaviors[BLOCK_GRAVEL].m_onBlockAdded = [](WorldManager& world, Int3 pos) -> void {
		world.m_tickScheduler.scheduleUpdateTick(pos, BLOCK_GRAVEL, 3);
	};
	blockBehaviors[BLOCK_GRAVEL].m_onTick = [](WorldManager& world, Int3 pos, uint8_t meta, Java::Random& random) -> void {
		Int3 below = { pos.m_x, pos.m_y - 1, pos.m_z };

		if (!Blocks::canFallAt(world, below) || pos.m_y < 0)
			return;

		constexpr int32_t checkRadius = 32; // Blocks
		bool areaLoaded = world.AABBinValidChunks({ double(pos.m_x - checkRadius), double(pos.m_y - checkRadius),
		                                            double(pos.m_z - checkRadius), double(pos.m_x + checkRadius),
		                                            double(pos.m_y + checkRadius), double(pos.m_z + checkRadius) });

		if (areaLoaded) {
			Vec3 spawnPos = { pos.m_x + 0.5, pos.m_y + 0.5, pos.m_z + 0.5 };
			auto entity = std::make_shared<FallingBlockEntity>(spawnPos, BLOCK_GRAVEL);
			world.m_entityManager.addEntity(std::move(entity));
			world.setBlock(pos, BLOCK_AIR, 0);
		} else {
			world.setBlock(pos, BLOCK_AIR, 0);

			Int3 landing = pos;
			while (Blocks::canFallAt(world, { landing.m_x, landing.m_y - 1, landing.m_z }) && landing.m_y > 0)
				landing.m_y--;

			if (landing.m_y > 0)
				world.setBlock(landing, BLOCK_GRAVEL, 0);
		}
	};
	blockBehaviors[BLOCK_SAND].m_onNeighborBlockChange = [](WorldManager& world, Int3 pos) -> void {
		// Schedule a check to see if we can fall
		world.m_tickScheduler.scheduleUpdateTick(pos, BLOCK_SAND, 3);
	};
	blockBehaviors[BLOCK_SAND].m_onBlockAdded = [](WorldManager& world, Int3 pos) -> void {
		world.m_tickScheduler.scheduleUpdateTick(pos, BLOCK_SAND, 3);
	};
	blockBehaviors[BLOCK_SAND].m_onTick = [](WorldManager& world, Int3 pos, uint8_t meta, Java::Random& random) -> void {
		Int3 below = { pos.m_x, pos.m_y - 1, pos.m_z };

		if (!Blocks::canFallAt(world, below) || pos.m_y < 0)
			return;

		constexpr int32_t checkRadius = 32; // Blocks
		bool areaLoaded = world.AABBinValidChunks({ double(pos.m_x - checkRadius), double(pos.m_y - checkRadius),
		                                            double(pos.m_z - checkRadius), double(pos.m_x + checkRadius),
		                                            double(pos.m_y + checkRadius), double(pos.m_z + checkRadius) });

		if (areaLoaded) {
			Vec3 spawnPos = { pos.m_x + 0.5, pos.m_y + 0.5, pos.m_z + 0.5 };
			auto entity = std::make_shared<FallingBlockEntity>(spawnPos, BLOCK_SAND);
			world.m_entityManager.addEntity(std::move(entity));
			world.setBlock(pos, BLOCK_AIR, 0);
		} else {
			world.setBlock(pos, BLOCK_AIR, 0);

			Int3 landing = pos;
			while (Blocks::canFallAt(world, { landing.m_x, landing.m_y - 1, landing.m_z }) && landing.m_y > 0)
				landing.m_y--;

			if (landing.m_y > 0)
				world.setBlock(landing, BLOCK_SAND, 0);
		}
	};
	blockBehaviors[BLOCK_CHEST].m_onBlockAdded = [](WorldManager& world, Int3 pos) -> void {
		auto chest = std::make_shared<TileEntityChest>(pos);
		world.createTileEntity(std::move(chest));
	};

	// --------------- block drops, only exceptions are included (something that doesn't drop itself) ---------------
	blockBehaviors[BLOCK_STONE].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_COBBLESTONE;
	};
	blockBehaviors[BLOCK_GRASS].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_DIRT;
	};
	blockBehaviors[BLOCK_FARMLAND].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_DIRT;
	};
	blockBehaviors[BLOCK_ORE_COAL].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::COAL;
	};
	blockBehaviors[BLOCK_ORE_DIAMOND].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::DIAMOND;
	};
	blockBehaviors[BLOCK_REDSTONE].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE;
	};
	blockBehaviors[BLOCK_SUGARCANE].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::SUGARCANE;
	};
	blockBehaviors[BLOCK_COBWEB].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::STRING;
	};
	blockBehaviors[BLOCK_DEADBUSH].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::INVALID;
	};
	blockBehaviors[BLOCK_STAIRS_WOOD].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_PLANKS;
	};
	blockBehaviors[BLOCK_STAIRS_COBBLESTONE].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_COBBLESTONE;
	};

	blockBehaviors[BLOCK_SIGN].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::SIGN;
	};
	blockBehaviors[BLOCK_SIGN_WALL].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::SIGN;
	};
	blockBehaviors[BLOCK_FURNACE_LIT].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_FURNACE;
	};
	blockBehaviors[BLOCK_REDSTONE_REPEATER_OFF].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE_REPEATER;
	};
	blockBehaviors[BLOCK_REDSTONE_REPEATER_ON].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE_REPEATER;
	};
	blockBehaviors[BLOCK_REDSTONE_TORCH_OFF].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_REDSTONE_TORCH_ON;
	};

	// --------------- drop themselves but pass their metadata onto the item ---------------
	blockBehaviors[BLOCK_WOOL].m_damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta;
	};
	blockBehaviors[BLOCK_LOG].m_damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta;
	};
	blockBehaviors[BLOCK_SAPLING].m_damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta & 3;
	};

	// --------------- don't drop anything ---------------
	blockBehaviors[BLOCK_ICE].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_GLASS].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_BOOKSHELF].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_CAKE].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_MOB_SPAWNER].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_FIRE].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_PISTON_HEAD].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_PISTON_MOVING].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_NETHER_PORTAL].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};
	blockBehaviors[BLOCK_SNOW_LAYER].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 0;
	};

	// --------------- drops influenced by RNG ---------------
	blockBehaviors[BLOCK_GRAVEL].m_idDropped = [](uint8_t, Java::Random& rng) -> ItemId {
		return rng.nextInt(10) == 0 ? static_cast<ItemId>(Items::Id::FLINT) : static_cast<ItemId>(BLOCK_GRAVEL);
	};

	blockBehaviors[BLOCK_ORE_LAPIS_LAZULI].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::DYE;
	};
	blockBehaviors[BLOCK_ORE_LAPIS_LAZULI].m_damageDropped = [](uint8_t) -> ItemDamage {
		return 4;
	};
	blockBehaviors[BLOCK_ORE_LAPIS_LAZULI].m_quantityDropped = [](Java::Random& rng) -> ItemAmount {
		return 4 + rng.nextInt(5);
	};

	blockBehaviors[BLOCK_ORE_REDSTONE_OFF].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE;
	};
	blockBehaviors[BLOCK_ORE_REDSTONE_OFF].m_quantityDropped = [](Java::Random& rng) -> ItemAmount {
		return 4 + rng.nextInt(2);
	};
	blockBehaviors[BLOCK_ORE_REDSTONE_ON].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::REDSTONE;
	};
	blockBehaviors[BLOCK_ORE_REDSTONE_ON].m_quantityDropped = [](Java::Random& rng) -> ItemAmount {
		return 4 + rng.nextInt(2);
	};

	blockBehaviors[BLOCK_GLOWSTONE].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::GLOWSTONE_DUST;
	};
	blockBehaviors[BLOCK_GLOWSTONE].m_quantityDropped = [](Java::Random& rng) -> ItemAmount {
		return 2 + rng.nextInt(3);
	};

	blockBehaviors[BLOCK_LEAVES].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_SAPLING;
	};
	blockBehaviors[BLOCK_LEAVES].m_damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta & 3;
	};
	blockBehaviors[BLOCK_LEAVES].m_quantityDropped = [](Java::Random& rng) -> ItemAmount {
		return rng.nextInt(20) == 0 ? 1 : 0;
	};

	blockBehaviors[BLOCK_TALLGRASS].m_idDropped = [](uint8_t, Java::Random& rng) -> ItemId {
		return rng.nextInt(8) == 0 ? Items::Id::SEEDS_WHEAT : Items::Id::INVALID;
	};

	// --------------- only drop if it's the correct half of the block being broken ---------------
	// TODO: other half of the block should be removed automatically
	blockBehaviors[BLOCK_DOOR_WOOD].m_idDropped = [](uint8_t meta, Java::Random&) -> ItemId {
		return (meta & 8) != 0 ? Items::Id::INVALID : Items::Id::DOOR_WOOD;
	};

	blockBehaviors[BLOCK_DOOR_IRON].m_idDropped = [](uint8_t meta, Java::Random&) -> ItemId {
		return (meta & 8) != 0 ? Items::Id::INVALID : Items::Id::DOOR_IRON;
	};

	blockBehaviors[BLOCK_BED].m_idDropped = [](uint8_t meta, Java::Random&) -> ItemId {
		return (meta & 8) != 0 ? Items::Id::INVALID : Items::Id::BED;
	};

	blockBehaviors[BLOCK_CLAY].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::CLAY;
	};
	blockBehaviors[BLOCK_CLAY].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 4;
	};

	blockBehaviors[BLOCK_SLAB].m_damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta;
	};

	blockBehaviors[BLOCK_DOUBLE_SLAB].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return BLOCK_SLAB;
	};
	blockBehaviors[BLOCK_DOUBLE_SLAB].m_damageDropped = [](uint8_t meta) -> ItemDamage {
		return meta;
	};
	blockBehaviors[BLOCK_DOUBLE_SLAB].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 2;
	};

	blockBehaviors[BLOCK_SNOW].m_idDropped = [](uint8_t, Java::Random&) -> ItemId {
		return Items::Id::SNOWBALL;
	};
	blockBehaviors[BLOCK_SNOW].m_quantityDropped = [](Java::Random&) -> ItemAmount {
		return 4;
	};
}

} // namespace Blocks