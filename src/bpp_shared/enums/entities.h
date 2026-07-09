/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include <cstdint>

enum class EntityType : int8_t {
	// Misc
	NONE,
	ITEM,
	PLAYER,
	FISH,
	FIREBALL,
	// Objects
	BOAT,
	MINECART,
	STORAGE_MINECART,
	FURNACE_MINECART,
	LIT_TNT,
	ARROW,
	THROWN_SNOWBALL,
	THROWN_EGG,
	FALLING_SAND,
	FALLING_GRAVEL,
	FISHING_BOBBER,
	// Mobs
	CREEPER,
	SKELETON,
	SPIDER,
	GIANT_ZOMBIE,
	ZOMBIE,
	SLIME,
	GHAST,
	ZOMBIE_PIGMAN,
	PIG,
	SHEEP,
	COW,
	CHICKEN,
	SQUID,
	WOLF,
	PAINTING,
};