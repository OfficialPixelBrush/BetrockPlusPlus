/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "materials.h"
#include "packet_data.h"
#include <numeric_structs.h>

struct WorldManager;
struct Entity;
struct PlayerSession;

namespace Blocks {

bool CanSugarcaneSurviveAt(WorldManager& _world, Int3 _pos);
bool CanTorchAttachTo(WorldManager& _world, Int3 _pos, PacketData::FaceDirection _face);
float GetFluidPercentAir(uint8_t _meta);
void BreakAndDropBlock(WorldManager& _world, Int3 _pos);
bool CanFallAt(WorldManager& _world, Int3 _position);

constexpr Int3 GetAdjacentBlockPos(Int3 _pos, PacketData::FaceDirection _face) {
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

constexpr Int3 GetSourceBlockFromFace(Int3 _pos, PacketData::FaceDirection _face) {
	switch (_face) {
	case PacketData::FaceDirection::Y_MINUS:
		++_pos.y;
		break;
	case PacketData::FaceDirection::Y_PLUS:
		--_pos.y;
		break;
	case PacketData::FaceDirection::Z_MINUS:
		++_pos.z;
		break;
	case PacketData::FaceDirection::Z_PLUS:
		--_pos.z;
		break;
	case PacketData::FaceDirection::X_MINUS:
		++_pos.x;
		break;
	case PacketData::FaceDirection::X_PLUS:
		--_pos.x;
		break;
	default:
		break;
	}
	return _pos;
}

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
	int tickRate = 10; // Used for self scheduling blocks

	float hardness = 1.0f;        // -1 = unbreakable (bedrock)
	float resistance = 5.0f;      // blast resistance
	float slipperiness = 0.6f;    // default friction, ice = 0.98f
	float particleGravity = 1.0f; // how fast break particles fall

	bool isCollidable = true;
	bool isOpaqueCube = true;
	bool isNormalCube = true;
	bool renderAsNormalBlock = true;
	bool ticksOnLoad = false; // Can we random tick?
	bool canBlockGrass = true;
	bool enableStats = true; // false = breaking doesn't count for achievements
	bool notifySelfOnMetaChange = true; // Whether to send an update to the client when meta changes
};

// Indexed by block ID, populated by registerAll()
extern BlockProperties blockProperties[256];

void RegisterBlockProperties();
} // namespace Blocks