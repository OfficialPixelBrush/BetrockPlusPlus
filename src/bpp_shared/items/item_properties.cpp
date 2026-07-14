/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "item_properties.h"
#include "enums/items.h"
#include "tile_entities/tile_entity.h"
#include "world/world.h"

namespace Items {

// Tool material durability
static constexpr ItemDamage DURABILITY_WOOD = 59;
static constexpr ItemDamage DURABILITY_STONE = 131;
static constexpr ItemDamage DURABILITY_IRON = 250;
static constexpr ItemDamage DURABILITY_DIAMOND = 1561;
static constexpr ItemDamage DURABILITY_GOLD = 32;

// Armor max damage = maxDamageArray[slot] * 3 << material
// material: leather=0, chain/gold=1, iron=2, diamond=3
// slot: helmet=0(x11), chest=1(x16), legs=2(x15), boots=3(x13)
static constexpr ItemDamage DURABILITY_HELMET_LEATHER = 11 * 3 * 1; // 33
static constexpr ItemDamage DURABILITY_CHEST_LEATHER = 16 * 3 * 1;  // 48
static constexpr ItemDamage DURABILITY_LEGS_LEATHER = 15 * 3 * 1;   // 45
static constexpr ItemDamage DURABILITY_BOOTS_LEATHER = 13 * 3 * 1;  // 39

static constexpr ItemDamage DURABILITY_HELMET_CHAINMAIL = 11 * 3 * 2; // 66
static constexpr ItemDamage DURABILITY_CHEST_CHAINMAIL = 16 * 3 * 2;  // 96
static constexpr ItemDamage DURABILITY_LEGS_CHAINMAIL = 15 * 3 * 2;   // 90
static constexpr ItemDamage DURABILITY_BOOTS_CHAINMAIL = 13 * 3 * 2;  // 78

static constexpr ItemDamage DURABILITY_HELMET_IRON = 11 * 3 * 4; // 132
static constexpr ItemDamage DURABILITY_CHEST_IRON = 16 * 3 * 4;  // 192
static constexpr ItemDamage DURABILITY_LEGS_IRON = 15 * 3 * 4;   // 180
static constexpr ItemDamage DURABILITY_BOOTS_IRON = 13 * 3 * 4;  // 156

static constexpr ItemDamage DURABILITY_HELMET_DIAMOND = 11 * 3 * 8; // 264
static constexpr ItemDamage DURABILITY_CHEST_DIAMOND = 16 * 3 * 8;  // 384
static constexpr ItemDamage DURABILITY_LEGS_DIAMOND = 15 * 3 * 8;   // 360
static constexpr ItemDamage DURABILITY_BOOTS_DIAMOND = 13 * 3 * 8;  // 312

static constexpr ItemDamage DURABILITY_HELMET_GOLD = 11 * 3 * 2; // 66
static constexpr ItemDamage DURABILITY_CHEST_GOLD = 16 * 3 * 2;  // 96
static constexpr ItemDamage DURABILITY_LEGS_GOLD = 15 * 3 * 2;   // 90
static constexpr ItemDamage DURABILITY_BOOTS_GOLD = 13 * 3 * 2;  // 78

static constexpr ItemDamage DURABILITY_FISHING_ROD = 64;
static constexpr ItemDamage DURABILITY_FLINT_AND_STEEL = 64;
static constexpr ItemDamage DURABILITY_SHEARS = 238;
static constexpr ItemDamage DURABILITY_BOW = 384;

// Global table definitions; declared extern in the header
std::unordered_map<ItemId, ItemBehavior> itemBehavior = {};
std::unordered_map<ItemId, ItemProperties> itemProperties = {};
std::unordered_map<ItemId, ToolProperties> toolProperties = {};

bool IsValid(ItemId id) {
	return ((id >= Items::Id::SHOVEL_IRON && id < Items::Id::MAX) ||
	        (id >= Items::Id::RECORD_13 && id < Items::Id::RECORD_MAX));
}

bool IsArmor(ItemId id) {
	return (id >= Items::HELMET_LEATHER && id <= Items::BOOTS_GOLD);
}

bool IsHoe(ItemId id) {
	return (id >= Items::HOE_WOOD && id <= Items::HOE_GOLD);
}

bool IsSword(ItemId id) {
	return (id == Items::SWORD_IRON || id == Items::SWORD_WOOD || id == Items::SWORD_STONE ||
	        id == Items::SWORD_DIAMOND || id == Items::SWORD_GOLD);
}

bool IsPickaxe(ItemId id) {
	return (id == Items::PICKAXE_IRON || id == Items::PICKAXE_WOOD || id == Items::PICKAXE_STONE ||
	        id == Items::PICKAXE_DIAMOND || id == Items::PICKAXE_GOLD);
}

bool IsAxe(ItemId id) {
	return (id == Items::AXE_IRON || id == Items::AXE_WOOD || id == Items::AXE_STONE || id == Items::AXE_DIAMOND ||
	        id == Items::AXE_GOLD);
}

bool IsShovel(ItemId id) {
	return (id == Items::SHOVEL_IRON || id == Items::SHOVEL_WOOD || id == Items::SHOVEL_STONE ||
	        id == Items::SHOVEL_DIAMOND || id == Items::SHOVEL_GOLD);
}

bool IsWeapon(ItemId id) {
	return IsSword(id) || id == Items::BOW;
}

bool IsTool(ItemId id) {
	return IsShovel(id) || IsAxe(id) || IsPickaxe(id) || IsHoe(id) || id == Items::FLINT_AND_STEEL ||
	       id == Items::FISHING_ROD || id == Items::SHEARS;
}

bool IsThrowable(ItemId id) {
	return (id == Items::SNOWBALL || id == Items::EGG);
}

bool IsBlock(ItemId id) {
	return (id > 0 && id <= Items::THRESHOLD);
}

bool IsStackable(ItemId id) {
	return Items::GetMaxStack(id) > 1;
}

int32_t GetMaxStack(ItemId id) {
	// Stack size 1
	switch (id) {
		// Food (ItemFood sets maxStackSize=1 in constructor)
	case Items::APPLE:
	case Items::APPLE_GOLDEN:
	case Items::BREAD:
	case Items::PORKCHOP:
	case Items::PORKCHOP_COOKED:
	case Items::FISH:
	case Items::FISH_COOKED:
	case Items::MUSHROOM_STEW: // ItemSoup extends ItemFood

		// Containers / vehicles / misc unstackables
	case Items::CAKE: // ItemReed.setMaxStackSize(1)
	case Items::BED:  // ItemBed.setMaxStackSize(1)
	case Items::SADDLE:
	case Items::BUCKET:
	case Items::BUCKET_WATER:
	case Items::BUCKET_LAVA:
	case Items::BUCKET_MILK:
	case Items::MINECART:
	case Items::MINECART_CHEST:
	case Items::MINECART_FURNACE:
	case Items::BOAT:
	case Items::DOOR_WOOD:
	case Items::DOOR_IRON:
	case Items::SIGN:       // ItemSign
	case Items::MAP:        // ItemMap.setMaxStackSize(1)
	case Items::RECORD_13:  // ItemRecord
	case Items::RECORD_CAT: // ItemRecord
		return 1;

	default:
		break;
	}

	// Tools, weapons, armor all set maxStackSize=1 in their constructors
	if (IsTool(id) || IsWeapon(id) || IsArmor(id))
		return 1;

	// Stack size 16
	if (id == Items::SNOWBALL || id == Items::EGG)
		return 16;

	if (id == Items::COOKIE)
		return 8;

	// Item, ItemCoal, ItemSeeds, ItemRedstone, ItemDye, ItemPainting,
	// ItemReed (sugarcane & repeater item), ItemRecord (never reached above),
	// all blocks, and any resource item not listed above.
	return Items::STACK_MAX;
}

ItemDamage GetMaxDurability(ItemId id) {
	switch (id) {
		// Swords
	case Items::SWORD_WOOD:
		return Items::DURABILITY_WOOD;
	case Items::SWORD_STONE:
		return Items::DURABILITY_STONE;
	case Items::SWORD_IRON:
		return Items::DURABILITY_IRON;
	case Items::SWORD_DIAMOND:
		return Items::DURABILITY_DIAMOND;
	case Items::SWORD_GOLD:
		return Items::DURABILITY_GOLD;

		// Shovels
	case Items::SHOVEL_WOOD:
		return Items::DURABILITY_WOOD;
	case Items::SHOVEL_STONE:
		return Items::DURABILITY_STONE;
	case Items::SHOVEL_IRON:
		return Items::DURABILITY_IRON;
	case Items::SHOVEL_DIAMOND:
		return Items::DURABILITY_DIAMOND;
	case Items::SHOVEL_GOLD:
		return Items::DURABILITY_GOLD;

		// Pickaxes
	case Items::PICKAXE_WOOD:
		return Items::DURABILITY_WOOD;
	case Items::PICKAXE_STONE:
		return Items::DURABILITY_STONE;
	case Items::PICKAXE_IRON:
		return Items::DURABILITY_IRON;
	case Items::PICKAXE_DIAMOND:
		return Items::DURABILITY_DIAMOND;
	case Items::PICKAXE_GOLD:
		return Items::DURABILITY_GOLD;

		// Axes
	case Items::AXE_WOOD:
		return Items::DURABILITY_WOOD;
	case Items::AXE_STONE:
		return Items::DURABILITY_STONE;
	case Items::AXE_IRON:
		return Items::DURABILITY_IRON;
	case Items::AXE_DIAMOND:
		return Items::DURABILITY_DIAMOND;
	case Items::AXE_GOLD:
		return Items::DURABILITY_GOLD;

		// Hoes
	case Items::HOE_WOOD:
		return Items::DURABILITY_WOOD;
	case Items::HOE_STONE:
		return Items::DURABILITY_STONE;
	case Items::HOE_IRON:
		return Items::DURABILITY_IRON;
	case Items::HOE_DIAMOND:
		return Items::DURABILITY_DIAMOND;
	case Items::HOE_GOLD:
		return Items::DURABILITY_GOLD;

		// Armor - Leather
	case Items::HELMET_LEATHER:
		return Items::DURABILITY_HELMET_LEATHER;
	case Items::CHESTPLATE_LEATHER:
		return Items::DURABILITY_CHEST_LEATHER;
	case Items::LEGGINGS_LEATHER:
		return Items::DURABILITY_LEGS_LEATHER;
	case Items::BOOTS_LEATHER:
		return Items::DURABILITY_BOOTS_LEATHER;

		// Armor - Chainmail
	case Items::HELMET_CHAINMAIL:
		return Items::DURABILITY_HELMET_CHAINMAIL;
	case Items::CHESTPLATE_CHAINMAIL:
		return Items::DURABILITY_CHEST_CHAINMAIL;
	case Items::LEGGINGS_CHAINMAIL:
		return Items::DURABILITY_LEGS_CHAINMAIL;
	case Items::BOOTS_CHAINMAIL:
		return Items::DURABILITY_BOOTS_CHAINMAIL;

		// Armor - Iron
	case Items::HELMET_IRON:
		return Items::DURABILITY_HELMET_IRON;
	case Items::CHESTPLATE_IRON:
		return Items::DURABILITY_CHEST_IRON;
	case Items::LEGGINGS_IRON:
		return Items::DURABILITY_LEGS_IRON;
	case Items::BOOTS_IRON:
		return Items::DURABILITY_BOOTS_IRON;

		// Armor - Diamond
	case Items::HELMET_DIAMOND:
		return Items::DURABILITY_HELMET_DIAMOND;
	case Items::CHESTPLATE_DIAMOND:
		return Items::DURABILITY_CHEST_DIAMOND;
	case Items::LEGGINGS_DIAMOND:
		return Items::DURABILITY_LEGS_DIAMOND;
	case Items::BOOTS_DIAMOND:
		return Items::DURABILITY_BOOTS_DIAMOND;

		// Armor - Gold
	case Items::HELMET_GOLD:
		return Items::DURABILITY_HELMET_GOLD;
	case Items::CHESTPLATE_GOLD:
		return Items::DURABILITY_CHEST_GOLD;
	case Items::LEGGINGS_GOLD:
		return Items::DURABILITY_LEGS_GOLD;
	case Items::BOOTS_GOLD:
		return Items::DURABILITY_BOOTS_GOLD;

		// Misc damageable
	case Items::FLINT_AND_STEEL:
		return Items::DURABILITY_FLINT_AND_STEEL;
	case Items::FISHING_ROD:
		return Items::DURABILITY_FISHING_ROD;
	case Items::SHEARS:
		return Items::DURABILITY_SHEARS;
	case Items::BOW:
		return Items::DURABILITY_BOW;

	default:
		return 0; // not damageable
	}
}
}; // namespace Items