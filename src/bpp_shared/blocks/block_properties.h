/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "direction.h"
#include "materials.h"
#include "packet_data.h"
#include <numeric_structs.h>

class WorldManager;
class WorldAccess;
struct Entity;
struct PlayerSession;

namespace Blocks {

bool CanSugarcaneSurviveAt(WorldAccess& _world, Int3 _pos);
bool CanCropsSurviveAt(WorldAccess& _world, Int3 _pos);
bool CanTorchAttachTo(WorldManager& _world, Int3 _pos, Direction::Value _face);
float GetFluidPercentAir(uint8_t _meta);
void BreakAndDropBlock(WorldManager& _world, Int3 _pos);
void BreakAndDropBlockWithChance(WorldManager& _world, Int3 _pos, float _chance);
void DropBlockAt(WorldManager& _world, Int3 _pos, BlockType _id, ItemAmount _count, int16_t _data);
void DropItemAt(WorldManager& _world, Int3 _pos, Items::Id _id, ItemAmount _count, int16_t _data);
bool CanFallAt(WorldAccess& _world, Int3 _position);
bool CanGenericPlantSurviveAt(WorldAccess& _world, Int3 _pos);
bool CanMushroomSurviveAt(WorldAccess& _world, Int3 _pos);
bool CanCactusSurviveAt(WorldAccess& _world, Int3 _pos);

enum class StepSound : uint8_t {
	Stone, // default, also metal (different pitch)
	Wood,
	Gravel,
	Grass,
	Sand,
	Cloth,
	Glass,
};

struct BlockProperties {
	Material material = Material::Rock();
	StepSound stepSound = StepSound::Stone;

	uint8_t lightEmission = 0;  // 0-15
	uint8_t lightOpacity = 255; // 0 = transparent, 255 = fully opaque
	int tickRate = 10;          // Used for self scheduling blocks

	float hardness = 1.0f;        // -1 = unbreakable (bedrock)
	float resistance = -1.0f;     // -1 = never explicitly set, fallback
	float slipperiness = 0.6f;    // default friction, ice = 0.98f
	float particleGravity = 1.0f; // how fast break particles fall

	bool isCollidable : 1 = true;
	bool isOpaqueCube : 1 = true;
	bool isNormalCube : 1 = true;
	bool renderAsNormalBlock : 1 = true;
	bool ticksOnLoad : 1 = false; // Can we random tick?
	bool canBlockGrass : 1 = true;
	bool enableStats : 1 = true;            // false = breaking doesn't count for achievements
	bool notifySelfOnMetaChange : 1 = true; // Whether to send an update to the client when meta changes
};

// Indexed by block ID, populated by registerAll()
extern BlockProperties blockProperties[BLOCK_MAX];

void RegisterBlockProperties();
} // namespace Blocks