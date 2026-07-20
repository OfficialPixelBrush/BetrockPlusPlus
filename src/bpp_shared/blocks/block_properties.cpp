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
static bool CanFallAt(WorldManager& _world, Int3 _position) {
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

Int3 GetAdjacentBlockPos(Int3 _pos, PacketData::FaceDirection _face) {
	switch (_face) {
	case PacketData::FaceDirection::Y_MINUS:
		--_pos.y;
		break;
	case PacketData::FaceDirection::Y_PLUS:
		++_pos.y;
		break;
	case PacketData::FaceDirection::Z_MINUS:
		--_pos.z;
		break;
	case PacketData::FaceDirection::Z_PLUS:
		++_pos.z;
		break;
	case PacketData::FaceDirection::X_MINUS:
		--_pos.x;
		break;
	case PacketData::FaceDirection::X_PLUS:
		++_pos.x;
		break;
	default:
		break;
	}
	return _pos;
}

bool CanSugarcaneSurviveAt(WorldManager& _world, Int3 _pos) {
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

// Some fluid specific stuff
float GetFluidPercentAir(uint8_t _meta) {
	if (_meta >= 8)
		_meta = 0;

	return float(_meta + 1) / 9.0f;
}

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

// defaults
static AABB DefaultAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
}
static CollisionShape DefaultCollider(uint8_t) {
	CollisionShape s;
	s.Add({ 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 });
	return s;
}

// slab
static AABB SlabAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 };
}
static CollisionShape SlabCollider(uint8_t) {
	CollisionShape s;
	s.Add({ 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 });
	return s;
}

// stairs
static CollisionShape StairCollider(uint8_t _meta) {
	CollisionShape s;
	switch (_meta & 3) {
	case 0:
		s.Add({ 0.0, 0.0, 0.0, 0.5, 0.5, 1.0 });
		s.Add({ 0.5, 0.0, 0.0, 1.0, 1.0, 1.0 });
		break;
	case 1:
		s.Add({ 0.0, 0.0, 0.0, 0.5, 1.0, 1.0 });
		s.Add({ 0.5, 0.0, 0.0, 1.0, 0.5, 1.0 });
		break;
	case 2:
		s.Add({ 0.0, 0.0, 0.0, 1.0, 0.5, 0.5 });
		s.Add({ 0.0, 0.0, 0.5, 1.0, 1.0, 1.0 });
		break;
	case 3:
		s.Add({ 0.0, 0.0, 0.0, 1.0, 1.0, 0.5 });
		s.Add({ 0.0, 0.0, 0.5, 1.0, 0.5, 1.0 });
		break;
	}
	return s;
}

// cactus
static AABB CactusAabb(uint8_t) {
	constexpr double I = 0.0625;
	return { I, 0.0, I, 1.0 - I, 1.0, 1.0 - I };
}
static CollisionShape CactusCollider(uint8_t) {
	constexpr double I = 0.0625;
	CollisionShape s;
	s.Add({ I, 0.0, I, 1.0 - I, 1.0 - I, 1.0 - I });
	return s;
}

// snow layer
static AABB SnowLayerAabb(uint8_t _meta) {
	float h = (2.0f * (1 + (_meta & 7))) / 16.0f;
	return { 0.0, 0.0, 0.0, 1.0, h, 1.0 };
}
static CollisionShape SnowLayerCollider(uint8_t _meta) {
	CollisionShape s;
	if ((_meta & 7) >= 3)
		s.Add({ 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 });
	return s;
}

// ladder
static AABB LadderAabb(uint8_t _meta) {
	constexpr double T = 0.125;
	switch (_meta) {
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
static CollisionShape LadderCollider(uint8_t _meta) {
	constexpr double T = 0.125;
	CollisionShape s;
	switch (_meta) {
	case 2:
		s.Add({ 0.0, 0.0, 1.0 - T, 1.0, 1.0, 1.0 });
		break;
	case 3:
		s.Add({ 0.0, 0.0, 0.0, 1.0, 1.0, T });
		break;
	case 4:
		s.Add({ 1.0 - T, 0.0, 0.0, 1.0, 1.0, 1.0 });
		break;
	case 5:
		s.Add({ 0.0, 0.0, 0.0, T, 1.0, 1.0 });
		break;
	}
	return s;
}

// door
// bits 0-1 = facing when closed, bit 2 = open, bit 3 = top half
static int DoorState(uint8_t _meta) {
	return ((_meta & 4) == 0) ? ((_meta - 1) & 3) : (_meta & 3);
}
static AABB DoorAabb(uint8_t _meta) {
	constexpr double T = 0.1875;
	switch (DoorState(_meta)) {
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
static CollisionShape DoorCollider(uint8_t _meta) {
	constexpr double T = 0.1875;
	CollisionShape s;
	switch (DoorState(_meta)) {
	case 0:
		s.Add({ 0.0, 0.0, 0.0, 1.0, 1.0, T });
		break;
	case 1:
		s.Add({ 1.0 - T, 0.0, 0.0, 1.0, 1.0, 1.0 });
		break;
	case 2:
		s.Add({ 0.0, 0.0, 1.0 - T, 1.0, 1.0, 1.0 });
		break;
	case 3:
		s.Add({ 0.0, 0.0, 0.0, T, 1.0, 1.0 });
		break;
	}
	return s;
}

// trapdoor
static AABB TrapdoorAabb(uint8_t _meta) {
	constexpr double T = 0.1875;
	if (!(_meta & 4))
		return { 0.0, 0.0, 0.0, 1.0, T, 1.0 };
	switch (_meta & 3) {
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

static CollisionShape FarmlandCollider(uint8_t) {
	CollisionShape s;
	s.Add({ 0.0, 0.0, 0.0, 1.0, 0.9375, 1.0 });
	return s;
}

static CollisionShape TrapdoorCollider(uint8_t _meta) {
	constexpr double T = 0.1875;
	CollisionShape s;
	if (!(_meta & 4)) {
		s.Add({ 0.0, 0.0, 0.0, 1.0, T, 1.0 });
		return s;
	}
	switch (_meta & 3) {
	case 0:
		s.Add({ 0.0, 0.0, 1.0 - T, 1.0, 1.0, 1.0 });
		break;
	case 1:
		s.Add({ 0.0, 0.0, 0.0, 1.0, 1.0, T });
		break;
	case 2:
		s.Add({ 1.0 - T, 0.0, 0.0, 1.0, 1.0, 1.0 });
		break;
	case 3:
		s.Add({ 0.0, 0.0, 0.0, T, 1.0, 1.0 });
		break;
	}
	return s;
}

// bed
static AABB BedAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.5625, 1.0 };
}
static CollisionShape BedCollider(uint8_t) {
	CollisionShape s;
	s.Add({ 0.0, 0.0, 0.0, 1.0, 0.5625, 1.0 });
	return s;
}

// fence
static CollisionShape FenceCollider(uint8_t) {
	CollisionShape s;
	s.Add({ 0.0, 0.0, 0.0, 1.0, 1.5, 1.0 });
	return s;
}

// cake
static AABB CakeAabb(uint8_t _meta) {
	double x0 = (1 + _meta * 2) / 16.0;
	return { x0, 0.0, 0.0625, 1.0 - 0.0625, 0.5 - 0.0625, 1.0 - 0.0625 };
}
static CollisionShape CakeCollider(uint8_t _meta) {
	double x0 = (1 + _meta * 2) / 16.0;
	CollisionShape s;
	s.Add({ x0, 0.0, 0.0625, 1.0 - 0.0625, 0.5 - 0.0625, 1.0 - 0.0625 });
	return s;
}

// repeater
static AABB RepeaterAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.125, 1.0 };
}
static CollisionShape EmptyCollider(uint8_t) {
	return {};
}

// button
static AABB ButtonAabb(uint8_t _meta) {
	const int face = _meta & 7;
	const bool pressed = (_meta & 8) != 0;
	constexpr double LO = 0.375, HI = 0.625, HW = 0.1875;
	const double depth = pressed ? 0.0625 : 0.125;
	switch (face) {
	case 1:
		return { 0.0, LO, 0.5 - HW, depth, HI, 0.5 + HW };
	case 2:
		return { 1.0 - depth, LO, 0.5 - HW, 1.0, HI, 0.5 + HW };
	case 3:
		return { 0.5 - HW, LO, 0.0, 0.5 + HW, HI, depth };
	case 4:
		return { 0.5 - HW, LO, 1.0 - depth, 0.5 + HW, HI, 1.0 };
	default:
		return {};
	}
}

// lever
static AABB LeverAabb(uint8_t _meta) {
	constexpr double F = 0.1875;
	switch (_meta & 7) {
	case 1:
		return { 0.0, 0.2, 0.5 - F, F * 2.0, 0.8, 0.5 + F };
	case 2:
		return { 1.0 - F * 2.0, 0.2, 0.5 - F, 1.0, 0.8, 0.5 + F };
	case 3:
		return { 0.5 - F, 0.2, 0.0, 0.5 + F, 0.8, F * 2.0 };
	case 4:
		return { 0.5 - F, 0.2, 1.0 - F * 2.0, 0.5 + F, 0.8, 1.0 };
	default: {
		constexpr double G = 0.25;
		return { 0.5 - G, 0.0, 0.5 - G, 0.5 + G, 0.6, 0.5 + G };
	}
	}
}

// pressure plate
static AABB PressurePlateAabb(uint8_t _meta) {
	constexpr double F = 0.0625;
	return { F, 0.0, F, 1.0 - F, (_meta == 1) ? 0.03125 : 0.0625, 1.0 - F };
}

// torch (normal + redstone, same box)
static AABB TorchAabb(uint8_t _meta) {
	constexpr double F = 0.15;
	switch (_meta & 7) {
	case 1:
		return { 0.0, 0.2, 0.5 - F, F * 2.0, 0.8, 0.5 + F };
	case 2:
		return { 1.0 - F * 2.0, 0.2, 0.5 - F, 1.0, 0.8, 0.5 + F };
	case 3:
		return { 0.5 - F, 0.2, 0.0, 0.5 + F, 0.8, F * 2.0 };
	case 4:
		return { 0.5 - F, 0.2, 1.0 - F * 2.0, 0.5 + F, 0.8, 1.0 };
	default: {
		constexpr double G = 0.1;
		return { 0.5 - G, 0.0, 0.5 - G, 0.5 + G, 0.6, 0.5 + G };
	}
	}
}

// rail
static AABB RailAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.125, 1.0 };
}

// redstone dust
static AABB RedstoneDustAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.0625, 1.0 };
}

// farmland
// Collider is full cube; ray/selection use visual height 0.937
static AABB FarmlandAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
}

// crop
static AABB CropAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.25, 1.0 }; // 4/16
}

// sapling / deadbush (f=0.4)
static AABB SaplingAabb(uint8_t) {
	constexpr float F = 0.4f;
	return { 0.5f - F, 0.0f, 0.5f - F, 0.5f + F, F * 2.0f, 0.5f + F };
}

// tall grass
static AABB TallGrassAabb(uint8_t) {
	constexpr float F = 0.4f;
	return { 0.5f - F, 0.0f, 0.5f - F, 0.5f + F, 0.8f, 0.5f + F };
}

// mushroom (f=0.2)
static AABB MushroomAabb(uint8_t) {
	constexpr float F = 0.2f;
	return { 0.5f - F, 0.0f, 0.5f - F, 0.5f + F, F * 2.0f, 0.5f + F };
}

// plant / flower (rose, dandelion) (f=0.2, h=f*3)
static AABB PlantAabb(uint8_t) {
	constexpr float F = 0.2f;
	return { 0.5f - F, 0.0f, 0.5f - F, 0.5f + F, F * 3.0f, 0.5f + F };
}

// sugarcane
static AABB SugarcaneAabb(uint8_t) {
	constexpr float F = 0.375f;
	return { 0.5f - F, 0.0f, 0.5f - F, 0.5f + F, 1.0f, 0.5f + F };
}

// Liquids have no collision
static AABB LiquidAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
}

// piston head
static AABB PistonHeadAabb(uint8_t _meta) {
	switch (_meta & 7) {
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
static CollisionShape PistonHeadCollider(uint8_t _meta) {
	CollisionShape s;
	switch (_meta & 7) {
	case 0:
		s.Add({ 0.0, 0.0, 0.0, 1.0, 0.25, 1.0 });
		s.Add({ 0.375, 0.25, 0.375, 0.625, 1.0, 0.625 });
		break;
	case 1:
		s.Add({ 0.0, 0.75, 0.0, 1.0, 1.0, 1.0 });
		s.Add({ 0.375, 0.0, 0.375, 0.625, 0.75, 0.625 });
		break;
	case 2:
		s.Add({ 0.0, 0.0, 0.0, 1.0, 1.0, 0.25 });
		s.Add({ 0.25, 0.375, 0.25, 0.75, 0.625, 1.0 });
		break;
	case 3:
		s.Add({ 0.0, 0.0, 0.75, 1.0, 1.0, 1.0 });
		s.Add({ 0.25, 0.375, 0.0, 0.75, 0.625, 0.75 });
		break;
	case 4:
		s.Add({ 0.0, 0.0, 0.0, 0.25, 1.0, 1.0 });
		s.Add({ 0.375, 0.25, 0.25, 0.625, 0.75, 1.0 });
		break;
	case 5:
		s.Add({ 0.75, 0.0, 0.0, 1.0, 1.0, 1.0 });
		s.Add({ 0.0, 0.375, 0.25, 0.75, 0.625, 0.75 });
		break;
	}
	return s;
}

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

void RegisterAll() {
	// Default all behavior slots to full-cube before per-block overrides
	for (int i = 0; i < 256; i++) {
		blockBehaviors[i].getSelectionBox = DefaultAabb;
		blockBehaviors[i].getRayBounds = DefaultAabb;
		blockBehaviors[i].getCollider = DefaultCollider;
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
		// ray/selection stay as defaultAABB (full cube is correct)
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
	blockBehaviors[BLOCK_CACTUS].onEntityCollidedWithBlock = [](WorldManager& _world, Int3 _pos, Entity& _entity) -> void {
		_entity.AttackEntityFrom(nullptr, 1);
	};
	blockBehaviors[BLOCK_COBWEB].onEntityCollidedWithBlock = [](WorldManager& _world, Int3 _pos, Entity& _entity) -> void {
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
	blockBehaviors[BLOCK_TORCH].onBlockPlaced = [](WorldManager& _world, Int3 _pos, Entity& _placer,
	                                               PacketData::FaceDirection _face) -> void {
		auto meta = _world.GetMetadata(_pos);
		if (_face == PacketData::FaceDirection::Y_PLUS && (_world.IsBlockNormalCube({ _pos.x, _pos.y - 1, _pos.z }) ||
		                                                  _world.GetBlockId({ _pos.x, _pos.y - 1, _pos.z }) == BLOCK_FENCE))
			meta = 5;
		if (_face == PacketData::FaceDirection::Z_MINUS && _world.IsBlockNormalCube({ _pos.x, _pos.y, _pos.z + 1 }))
			meta = 4;
		if (_face == PacketData::FaceDirection::Z_PLUS && _world.IsBlockNormalCube({ _pos.x, _pos.y, _pos.z - 1 }))
			meta = 3;
		if (_face == PacketData::FaceDirection::X_MINUS && _world.IsBlockNormalCube({ _pos.x + 1, _pos.y, _pos.z }))
			meta = 2;
		if (_face == PacketData::FaceDirection::X_PLUS && _world.IsBlockNormalCube({ _pos.x - 1, _pos.y, _pos.z }))
			meta = 1;
		_world.SetMeta(_pos, meta);
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
	blockBehaviors[BLOCK_GRAVEL].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta, Java::Random& _random) -> void {
		Int3 below = { _pos.x, _pos.y - 1, _pos.z };

		if (!Blocks::CanFallAt(_world, below) || _pos.y < 0)
			return;

		constexpr int32_t CHECK_RADIUS = 32; // Blocks
		bool areaLoaded = _world.AABBinValidChunks({ double(_pos.x - CHECK_RADIUS), double(_pos.y - CHECK_RADIUS),
		                                            double(_pos.z - CHECK_RADIUS), double(_pos.x + CHECK_RADIUS),
		                                            double(_pos.y + CHECK_RADIUS), double(_pos.z + CHECK_RADIUS) });

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
	blockBehaviors[BLOCK_SAND].onTick = [](WorldManager& _world, Int3 _pos, uint8_t _meta, Java::Random& _random) -> void {
		Int3 below = { _pos.x, _pos.y - 1, _pos.z };

		if (!Blocks::CanFallAt(_world, below) || _pos.y < 0)
			return;

		constexpr int32_t CHECK_RADIUS = 32; // Blocks
		bool areaLoaded = _world.AABBinValidChunks({ double(_pos.x - CHECK_RADIUS), double(_pos.y - CHECK_RADIUS),
		                                            double(_pos.z - CHECK_RADIUS), double(_pos.x + CHECK_RADIUS),
		                                            double(_pos.y + CHECK_RADIUS), double(_pos.z + CHECK_RADIUS) });

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
}

} // namespace Blocks