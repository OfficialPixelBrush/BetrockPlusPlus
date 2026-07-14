/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include "base_types.h"
#include <cstdint>

namespace Items {
// Items above this ID are pure items (not placeable blocks)
static constexpr int16_t THRESHOLD = 255;
// Maximum number of items in a stack
static constexpr int8_t STACK_MAX = 64;
enum Id : ItemId::Underlying {
	INVALID = -1,
	NONE = 0, // This is usually not you want to use, use INVALID instead

	// Tools - Iron
	SHOVEL_IRON = 256,
	PICKAXE_IRON = 257,
	AXE_IRON = 258,
	FLINT_AND_STEEL = 259,

	// Food
	APPLE = 260,

	// Combat
	BOW = 261,
	ARROW = 262,

	// Resources
	COAL = 263,
	DIAMOND = 264,
	IRON = 265,
	GOLD = 266,

	// Tools/Weapons - Iron
	SWORD_IRON = 267,

	// Tools/Weapons - Wood
	SWORD_WOOD = 268,
	SHOVEL_WOOD = 269,
	PICKAXE_WOOD = 270,
	AXE_WOOD = 271,

	// Tools/Weapons - Stone
	SWORD_STONE = 272,
	SHOVEL_STONE = 273,
	PICKAXE_STONE = 274,
	AXE_STONE = 275,

	// Tools/Weapons - Diamond
	SWORD_DIAMOND = 276,
	SHOVEL_DIAMOND = 277,
	PICKAXE_DIAMOND = 278,
	AXE_DIAMOND = 279,

	// Misc
	STICK = 280,
	BOWL = 281,
	MUSHROOM_STEW = 282,

	// Tools/Weapons - Gold
	SWORD_GOLD = 283,
	SHOVEL_GOLD = 284,
	PICKAXE_GOLD = 285,
	AXE_GOLD = 286,

	// Resources
	STRING = 287,
	FEATHER = 288,
	GUNPOWDER = 289,

	// Hoes
	HOE_WOOD = 290,
	HOE_STONE = 291,
	HOE_IRON = 292,
	HOE_DIAMOND = 293,
	HOE_GOLD = 294,

	// Farming
	SEEDS_WHEAT = 295,
	WHEAT = 296,

	// Food
	BREAD = 297,

	// Armor - Leather
	HELMET_LEATHER = 298,
	CHESTPLATE_LEATHER = 299,
	LEGGINGS_LEATHER = 300,
	BOOTS_LEATHER = 301,

	// Armor - Chainmail
	HELMET_CHAINMAIL = 302,
	CHESTPLATE_CHAINMAIL = 303,
	LEGGINGS_CHAINMAIL = 304,
	BOOTS_CHAINMAIL = 305,

	// Armor - Iron
	HELMET_IRON = 306,
	CHESTPLATE_IRON = 307,
	LEGGINGS_IRON = 308,
	BOOTS_IRON = 309,

	// Armor - Diamond
	HELMET_DIAMOND = 310,
	CHESTPLATE_DIAMOND = 311,
	LEGGINGS_DIAMOND = 312,
	BOOTS_DIAMOND = 313,

	// Armor - Gold
	HELMET_GOLD = 314,
	CHESTPLATE_GOLD = 315,
	LEGGINGS_GOLD = 316,
	BOOTS_GOLD = 317,

	// Resources/Food
	FLINT = 318,
	PORKCHOP = 319,
	PORKCHOP_COOKED = 320,
	PAINTING = 321,
	APPLE_GOLDEN = 322,

	// Placeable items
	SIGN = 323,
	DOOR_WOOD = 324,

	// Buckets
	BUCKET = 325,
	BUCKET_WATER = 326,
	BUCKET_LAVA = 327,

	// Vehicles
	MINECART = 328,
	SADDLE = 329,

	// Misc
	DOOR_IRON = 330,
	REDSTONE = 331,
	SNOWBALL = 332,
	BOAT = 333,
	LEATHER = 334,
	BUCKET_MILK = 335,
	BRICK = 336,
	CLAY = 337,
	SUGARCANE = 338,
	PAPER = 339,
	BOOK = 340,
	SLIME = 341,
	MINECART_CHEST = 342,
	MINECART_FURNACE = 343,
	EGG = 344,
	COMPASS = 345,
	FISHING_ROD = 346,
	CLOCK = 347,
	GLOWSTONE_DUST = 348,
	FISH = 349,
	FISH_COOKED = 350,
	DYE = 351,
	BONE = 352,
	SUGAR = 353,
	CAKE = 354,
	BED = 355,
	REDSTONE_REPEATER = 356,
	COOKIE = 357,
	MAP = 358,
	SHEARS = 359,
	MAX,

	// Records
	RECORD_13 = 2256,
	RECORD_CAT = 2257,
	RECORD_MAX,
};
}; // namespace Items