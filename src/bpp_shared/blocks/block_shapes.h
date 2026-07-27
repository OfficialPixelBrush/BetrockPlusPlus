/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "AABB.h"
#include <cstdint>

namespace Blocks {

// defaults
[[maybe_unused]] static AABB DefaultAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
}
[[maybe_unused]] static CollisionShape DefaultCollider(uint8_t) {
	CollisionShape s;
	s.Add({ 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 });
	return s;
}

// slab
[[maybe_unused]] static AABB SlabAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 };
}
[[maybe_unused]] static CollisionShape SlabCollider(uint8_t) {
	CollisionShape s;
	s.Add({ 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 });
	return s;
}

// stairs
[[maybe_unused]] static CollisionShape StairCollider(uint8_t _meta) {
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
[[maybe_unused]] static AABB CactusAabb(uint8_t) {
	constexpr double I = 0.0625;
	return { I, 0.0, I, 1.0 - I, 1.0, 1.0 - I };
}
[[maybe_unused]] static CollisionShape CactusCollider(uint8_t) {
	constexpr double I = 0.0625;
	CollisionShape s;
	s.Add({ I, 0.0, I, 1.0 - I, 1.0 - I, 1.0 - I });
	return s;
}

// snow layer
[[maybe_unused]] static AABB SnowLayerAabb(uint8_t _meta) {
	float h = (2.0f * (1 + (_meta & 7))) / 16.0f;
	return { 0.0, 0.0, 0.0, 1.0, h, 1.0 };
}
[[maybe_unused]] static CollisionShape SnowLayerCollider(uint8_t _meta) {
	CollisionShape s;
	if ((_meta & 7) >= 3)
		s.Add({ 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 });
	return s;
}

// ladder
[[maybe_unused]] static AABB LadderAabb(uint8_t _meta) {
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
[[maybe_unused]] static CollisionShape LadderCollider(uint8_t _meta) {
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
[[maybe_unused]] static int DoorState(uint8_t _meta) {
	return ((_meta & 4) == 0) ? ((_meta - 1) & 3) : (_meta & 3);
}
[[maybe_unused]] static AABB DoorAabb(uint8_t _meta) {
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
[[maybe_unused]] static CollisionShape DoorCollider(uint8_t _meta) {
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
[[maybe_unused]] static AABB TrapdoorAabb(uint8_t _meta) {
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

[[maybe_unused]] static CollisionShape FarmlandCollider(uint8_t) {
	CollisionShape s;
	s.Add({ 0.0, 0.0, 0.0, 1.0, 0.9375, 1.0 });
	return s;
}

[[maybe_unused]] static CollisionShape TrapdoorCollider(uint8_t _meta) {
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
[[maybe_unused]] static AABB BedAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.5625, 1.0 };
}
[[maybe_unused]] static CollisionShape BedCollider(uint8_t) {
	CollisionShape s;
	s.Add({ 0.0, 0.0, 0.0, 1.0, 0.5625, 1.0 });
	return s;
}

// fence
[[maybe_unused]] static CollisionShape FenceCollider(uint8_t) {
	CollisionShape s;
	s.Add({ 0.0, 0.0, 0.0, 1.0, 1.5, 1.0 });
	return s;
}

// cake
[[maybe_unused]] static AABB CakeAabb(uint8_t _meta) {
	double x0 = (1 + _meta * 2) / 16.0;
	return { x0, 0.0, 0.0625, 1.0 - 0.0625, 0.5 - 0.0625, 1.0 - 0.0625 };
}
[[maybe_unused]] static CollisionShape CakeCollider(uint8_t _meta) {
	double x0 = (1 + _meta * 2) / 16.0;
	CollisionShape s;
	s.Add({ x0, 0.0, 0.0625, 1.0 - 0.0625, 0.5 - 0.0625, 1.0 - 0.0625 });
	return s;
}

// repeater
[[maybe_unused]] static AABB RepeaterAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.125, 1.0 };
}
[[maybe_unused]] static CollisionShape EmptyCollider(uint8_t) {
	return {};
}

// button
[[maybe_unused]] static AABB ButtonAabb(uint8_t _meta) {
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
[[maybe_unused]] static AABB LeverAabb(uint8_t _meta) {
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
[[maybe_unused]] static AABB PressurePlateAabb(uint8_t _meta) {
	constexpr double F = 0.0625;
	return { F, 0.0, F, 1.0 - F, (_meta == 1) ? 0.03125 : 0.0625, 1.0 - F };
}

// torch (normal + redstone, same box)
[[maybe_unused]] static AABB TorchAabb(uint8_t _meta) {
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
[[maybe_unused]] static AABB RailAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.125, 1.0 };
}

// redstone dust
[[maybe_unused]] static AABB RedstoneDustAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.0625, 1.0 };
}

// farmland
// Collider is full cube; ray/selection use visual height 0.937
[[maybe_unused]] static AABB FarmlandAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
}

// crop
[[maybe_unused]] static AABB CropAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 1.0, 0.25, 1.0 }; // 4/16
}

// sapling / deadbush (f=0.4)
[[maybe_unused]] static AABB SaplingAabb(uint8_t) {
	constexpr float F = 0.4f;
	return { 0.5f - F, 0.0f, 0.5f - F, 0.5f + F, F * 2.0f, 0.5f + F };
}

// tall grass
[[maybe_unused]] static AABB TallGrassAabb(uint8_t) {
	constexpr float F = 0.4f;
	return { 0.5f - F, 0.0f, 0.5f - F, 0.5f + F, 0.8f, 0.5f + F };
}

// mushroom (f=0.2)
[[maybe_unused]] static AABB MushroomAabb(uint8_t) {
	constexpr float F = 0.2f;
	return { 0.5f - F, 0.0f, 0.5f - F, 0.5f + F, F * 2.0f, 0.5f + F };
}

// plant / flower (rose, dandelion) (f=0.2, h=f*3)
[[maybe_unused]] static AABB PlantAabb(uint8_t) {
	constexpr float F = 0.2f;
	return { 0.5f - F, 0.0f, 0.5f - F, 0.5f + F, F * 3.0f, 0.5f + F };
}

// sugarcane
[[maybe_unused]] static AABB SugarcaneAabb(uint8_t) {
	constexpr float F = 0.375f;
	return { 0.5f - F, 0.0f, 0.5f - F, 0.5f + F, 1.0f, 0.5f + F };
}

// Liquids have no collision
[[maybe_unused]] static AABB LiquidAabb(uint8_t) {
	return { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
}

// Soul sand is indented 1 block
[[maybe_unused]] static CollisionShape SoulSandCollider(uint8_t) {
	CollisionShape collider;
	collider.Add({ 0.0, 0.0, 0.0, 1.0, 0.875, 1.0 });
	return collider;
}

// piston head
[[maybe_unused]] static AABB PistonHeadAabb(uint8_t _meta) {
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
[[maybe_unused]] static CollisionShape PistonHeadCollider(uint8_t _meta) {
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
}; // namespace Blocks