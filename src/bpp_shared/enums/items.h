/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#pragma once
#include <cstdint>

// Items above this ID are pure items (not placeable blocks)
static constexpr int16_t ITEM_THRESHOLD = 255;
// Maximum number of items in a stack
static constexpr int8_t ITEM_STACK_MAX = 64;

enum Items : int16_t {
    ITEM_INVALID = -1,
    ITEM_NONE = 0,

    // Tools - Iron
    ITEM_SHOVEL_IRON = 256,
    ITEM_PICKAXE_IRON = 257,
    ITEM_AXE_IRON = 258,
    ITEM_FLINT_AND_STEEL = 259,

    // Food
    ITEM_APPLE = 260,

    // Combat
    ITEM_BOW = 261,
    ITEM_ARROW = 262,

    // Resources
    ITEM_COAL = 263,
    ITEM_DIAMOND = 264,
    ITEM_IRON = 265,
    ITEM_GOLD = 266,

    // Tools/Weapons - Iron
    ITEM_SWORD_IRON = 267,

    // Tools/Weapons - Wood
    ITEM_SWORD_WOOD = 268,
    ITEM_SHOVEL_WOOD = 269,
    ITEM_PICKAXE_WOOD = 270,
    ITEM_AXE_WOOD = 271,

    // Tools/Weapons - Stone
    ITEM_SWORD_STONE = 272,
    ITEM_SHOVEL_STONE = 273,
    ITEM_PICKAXE_STONE = 274,
    ITEM_AXE_STONE = 275,

    // Tools/Weapons - Diamond
    ITEM_SWORD_DIAMOND = 276,
    ITEM_SHOVEL_DIAMOND = 277,
    ITEM_PICKAXE_DIAMOND = 278,
    ITEM_AXE_DIAMOND = 279,

    // Misc
    ITEM_STICK = 280,
    ITEM_BOWL = 281,
    ITEM_MUSHROOM_STEW = 282,

    // Tools/Weapons - Gold
    ITEM_SWORD_GOLD = 283,
    ITEM_SHOVEL_GOLD = 284,
    ITEM_PICKAXE_GOLD = 285,
    ITEM_AXE_GOLD = 286,

    // Resources
    ITEM_STRING = 287,
    ITEM_FEATHER = 288,
    ITEM_GUNPOWDER = 289,

    // Hoes
    ITEM_HOE_WOOD = 290,
    ITEM_HOE_STONE = 291,
    ITEM_HOE_IRON = 292,
    ITEM_HOE_DIAMOND = 293,
    ITEM_HOE_GOLD = 294,

    // Farming
    ITEM_SEEDS_WHEAT = 295,
    ITEM_WHEAT = 296,

    // Food
    ITEM_BREAD = 297,

    // Armor - Leather
    ITEM_HELMET_LEATHER = 298,
    ITEM_CHESTPLATE_LEATHER = 299,
    ITEM_LEGGINGS_LEATHER = 300,
    ITEM_BOOTS_LEATHER = 301,

    // Armor - Chainmail
    ITEM_HELMET_CHAINMAIL = 302,
    ITEM_CHESTPLATE_CHAINMAIL = 303,
    ITEM_LEGGINGS_CHAINMAIL = 304,
    ITEM_BOOTS_CHAINMAIL = 305,

    // Armor - Iron
    ITEM_HELMET_IRON = 306,
    ITEM_CHESTPLATE_IRON = 307,
    ITEM_LEGGINGS_IRON = 308,
    ITEM_BOOTS_IRON = 309,

    // Armor - Diamond
    ITEM_HELMET_DIAMOND = 310,
    ITEM_CHESTPLATE_DIAMOND = 311,
    ITEM_LEGGINGS_DIAMOND = 312,
    ITEM_BOOTS_DIAMOND = 313,

    // Armor - Gold
    ITEM_HELMET_GOLD = 314,
    ITEM_CHESTPLATE_GOLD = 315,
    ITEM_LEGGINGS_GOLD = 316,
    ITEM_BOOTS_GOLD = 317,

    // Resources/Food
    ITEM_FLINT = 318,
    ITEM_PORKCHOP = 319,
    ITEM_PORKCHOP_COOKED = 320,
    ITEM_PAINTING = 321,
    ITEM_APPLE_GOLDEN = 322,

    // Placeable items
    ITEM_SIGN = 323,
    ITEM_DOOR_WOOD = 324,

    // Buckets
    ITEM_BUCKET = 325,
    ITEM_BUCKET_WATER = 326,
    ITEM_BUCKET_LAVA = 327,

    // Vehicles
    ITEM_MINECART = 328,
    ITEM_SADDLE = 329,

    // Misc
    ITEM_DOOR_IRON = 330,
    ITEM_REDSTONE = 331,
    ITEM_SNOWBALL = 332,
    ITEM_BOAT = 333,
    ITEM_LEATHER = 334,
    ITEM_BUCKET_MILK = 335,
    ITEM_BRICK = 336,
    ITEM_CLAY = 337,
    ITEM_SUGARCANE = 338,
    ITEM_PAPER = 339,
    ITEM_BOOK = 340,
    ITEM_SLIME = 341,
    ITEM_MINECART_CHEST = 342,
    ITEM_MINECART_FURNACE = 343,
    ITEM_EGG = 344,
    ITEM_COMPASS = 345,
    ITEM_FISHING_ROD = 346,
    ITEM_CLOCK = 347,
    ITEM_GLOWSTONE_DUST = 348,
    ITEM_FISH = 349,
    ITEM_FISH_COOKED = 350,
    ITEM_DYE = 351,
    ITEM_BONE = 352,
    ITEM_SUGAR = 353,
    ITEM_CAKE = 354,
    ITEM_BED = 355,
    ITEM_REDSTONE_REPEATER = 356,
    ITEM_COOKIE = 357,
    ITEM_MAP = 358,
    ITEM_SHEARS = 359,
    ITEM_MAX,

    // Records
    ITEM_RECORD_13 = 2256,
    ITEM_RECORD_CAT = 2257,
    ITEM_RECORD_MAX,
};

// Tool material durability
static constexpr int16_t DURABILITY_WOOD = 59;
static constexpr int16_t DURABILITY_STONE = 131;
static constexpr int16_t DURABILITY_IRON = 250;
static constexpr int16_t DURABILITY_DIAMOND = 1561;
static constexpr int16_t DURABILITY_GOLD = 32;

// Armor max damage = maxDamageArray[slot] * 3 << material
// material: leather=0, chain/gold=1, iron=2, diamond=3
// slot: helmet=0(x11), chest=1(x16), legs=2(x15), boots=3(x13)
static constexpr int16_t DURABILITY_HELMET_LEATHER = 11 * 3 * 1;  // 33
static constexpr int16_t DURABILITY_CHEST_LEATHER = 16 * 3 * 1;  // 48
static constexpr int16_t DURABILITY_LEGS_LEATHER = 15 * 3 * 1;  // 45
static constexpr int16_t DURABILITY_BOOTS_LEATHER = 13 * 3 * 1;  // 39

static constexpr int16_t DURABILITY_HELMET_CHAINMAIL = 11 * 3 * 2;  // 66
static constexpr int16_t DURABILITY_CHEST_CHAINMAIL = 16 * 3 * 2;  // 96
static constexpr int16_t DURABILITY_LEGS_CHAINMAIL = 15 * 3 * 2;  // 90
static constexpr int16_t DURABILITY_BOOTS_CHAINMAIL = 13 * 3 * 2;  // 78

static constexpr int16_t DURABILITY_HELMET_IRON = 11 * 3 * 4;  // 132
static constexpr int16_t DURABILITY_CHEST_IRON = 16 * 3 * 4;  // 192
static constexpr int16_t DURABILITY_LEGS_IRON = 15 * 3 * 4;  // 180
static constexpr int16_t DURABILITY_BOOTS_IRON = 13 * 3 * 4;  // 156

static constexpr int16_t DURABILITY_HELMET_DIAMOND = 11 * 3 * 8;  // 264
static constexpr int16_t DURABILITY_CHEST_DIAMOND = 16 * 3 * 8;  // 384
static constexpr int16_t DURABILITY_LEGS_DIAMOND = 15 * 3 * 8;  // 360
static constexpr int16_t DURABILITY_BOOTS_DIAMOND = 13 * 3 * 8;  // 312

static constexpr int16_t DURABILITY_HELMET_GOLD = 11 * 3 * 2;  // 66
static constexpr int16_t DURABILITY_CHEST_GOLD = 16 * 3 * 2;  // 96
static constexpr int16_t DURABILITY_LEGS_GOLD = 15 * 3 * 2;  // 90
static constexpr int16_t DURABILITY_BOOTS_GOLD = 13 * 3 * 2;  // 78

static constexpr int16_t DURABILITY_FISHING_ROD = 64;
static constexpr int16_t DURABILITY_FLINT_AND_STEEL = 64;
static constexpr int16_t DURABILITY_SHEARS = 238;
static constexpr int16_t DURABILITY_BOW = 384;

bool IsArmor(int16_t id);
bool IsHoe(int16_t id);
bool IsSword(int16_t id);
bool IsPickaxe(int16_t id);
bool IsAxe(int16_t id);
bool IsShovel(int16_t id);
bool IsWeapon(int16_t id);
bool IsTool(int16_t id);
bool IsThrowable(int16_t id);
bool IsStackable(int16_t id);  // max stack > 1
bool IsBlock(int16_t id);

// Returns max stack size for this item/block id
int32_t GetMaxStack(int16_t id);

// Returns max durability (0 = not damageable)
int16_t GetMaxDurability(int16_t id);