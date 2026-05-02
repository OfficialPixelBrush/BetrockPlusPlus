/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#include "items.h"

bool IsArmor(int16_t id) {
    return (id >= ITEM_HELMET_LEATHER && id <= ITEM_BOOTS_GOLD);
}

bool IsHoe(int16_t id) {
    return (id >= ITEM_HOE_WOOD && id <= ITEM_HOE_GOLD);
}

bool IsSword(int16_t id) {
    return (id == ITEM_SWORD_IRON || id == ITEM_SWORD_WOOD ||
        id == ITEM_SWORD_STONE || id == ITEM_SWORD_DIAMOND ||
        id == ITEM_SWORD_GOLD);
}

bool IsPickaxe(int16_t id) {
    return (id == ITEM_PICKAXE_IRON || id == ITEM_PICKAXE_WOOD ||
        id == ITEM_PICKAXE_STONE || id == ITEM_PICKAXE_DIAMOND ||
        id == ITEM_PICKAXE_GOLD);
}

bool IsAxe(int16_t id) {
    return (id == ITEM_AXE_IRON || id == ITEM_AXE_WOOD ||
        id == ITEM_AXE_STONE || id == ITEM_AXE_DIAMOND ||
        id == ITEM_AXE_GOLD);
}

bool IsShovel(int16_t id) {
    return (id == ITEM_SHOVEL_IRON || id == ITEM_SHOVEL_WOOD ||
        id == ITEM_SHOVEL_STONE || id == ITEM_SHOVEL_DIAMOND ||
        id == ITEM_SHOVEL_GOLD);
}

bool IsWeapon(int16_t id) {
    return IsSword(id) || id == ITEM_BOW;
}

bool IsTool(int16_t id) {
    return IsShovel(id) || IsAxe(id) || IsPickaxe(id) || IsHoe(id) ||
        id == ITEM_FLINT_AND_STEEL || id == ITEM_FISHING_ROD || id == ITEM_SHEARS;
}

bool IsThrowable(int16_t id) {
    return (id == ITEM_SNOWBALL || id == ITEM_EGG);
}

bool IsBlock(int16_t id) {
    return (id > 0 && id <= ITEM_THRESHOLD);
}

bool IsStackable(int16_t id) {
    return GetMaxStack(id) > 1;
}

int32_t GetMaxStack(int16_t id) {
    // Unstackable singles
    switch (id) {
    case ITEM_MUSHROOM_STEW:
    case ITEM_CAKE:
    case ITEM_BED:
    case ITEM_SADDLE:
    case ITEM_BUCKET:
    case ITEM_BUCKET_WATER:
    case ITEM_BUCKET_LAVA:
    case ITEM_BUCKET_MILK:
    case ITEM_MINECART:
    case ITEM_MINECART_CHEST:
    case ITEM_MINECART_FURNACE:
    case ITEM_BOAT:
    case ITEM_DOOR_WOOD:
    case ITEM_DOOR_IRON:
    case ITEM_RECORD_13:
    case ITEM_RECORD_CAT:
        return 1;
    default:
        break;
    }

    // Tools, weapons, armor — all stack to 1
    if (IsTool(id) || IsWeapon(id) || IsArmor(id))
        return 1;

    // Snowballs and eggs stack to 16
    if (id == ITEM_SNOWBALL || id == ITEM_EGG)
        return 16;

    // Everything else (blocks, resources, food) stacks to 64
    return 64;
}

int16_t GetMaxDurability(int16_t id) {
    switch (id) {
    // Swords
    case ITEM_SWORD_WOOD:       return DURABILITY_WOOD;
    case ITEM_SWORD_STONE:      return DURABILITY_STONE;
    case ITEM_SWORD_IRON:       return DURABILITY_IRON;
    case ITEM_SWORD_DIAMOND:    return DURABILITY_DIAMOND;
    case ITEM_SWORD_GOLD:       return DURABILITY_GOLD;

    // Shovels
    case ITEM_SHOVEL_WOOD:      return DURABILITY_WOOD;
    case ITEM_SHOVEL_STONE:     return DURABILITY_STONE;
    case ITEM_SHOVEL_IRON:      return DURABILITY_IRON;
    case ITEM_SHOVEL_DIAMOND:   return DURABILITY_DIAMOND;
    case ITEM_SHOVEL_GOLD:      return DURABILITY_GOLD;

    // Pickaxes
    case ITEM_PICKAXE_WOOD:     return DURABILITY_WOOD;
    case ITEM_PICKAXE_STONE:    return DURABILITY_STONE;
    case ITEM_PICKAXE_IRON:     return DURABILITY_IRON;
    case ITEM_PICKAXE_DIAMOND:  return DURABILITY_DIAMOND;
    case ITEM_PICKAXE_GOLD:     return DURABILITY_GOLD;

    // Axes
    case ITEM_AXE_WOOD:         return DURABILITY_WOOD;
    case ITEM_AXE_STONE:        return DURABILITY_STONE;
    case ITEM_AXE_IRON:         return DURABILITY_IRON;
    case ITEM_AXE_DIAMOND:      return DURABILITY_DIAMOND;
    case ITEM_AXE_GOLD:         return DURABILITY_GOLD;

    // Hoes
    case ITEM_HOE_WOOD:         return DURABILITY_WOOD;
    case ITEM_HOE_STONE:        return DURABILITY_STONE;
    case ITEM_HOE_IRON:         return DURABILITY_IRON;
    case ITEM_HOE_DIAMOND:      return DURABILITY_DIAMOND;
    case ITEM_HOE_GOLD:         return DURABILITY_GOLD;

    // Armor - Leather
    case ITEM_HELMET_LEATHER:       return DURABILITY_HELMET_LEATHER;
    case ITEM_CHESTPLATE_LEATHER:   return DURABILITY_CHEST_LEATHER;
    case ITEM_LEGGINGS_LEATHER:     return DURABILITY_LEGS_LEATHER;
    case ITEM_BOOTS_LEATHER:        return DURABILITY_BOOTS_LEATHER;

    // Armor - Chainmail
    case ITEM_HELMET_CHAINMAIL:     return DURABILITY_HELMET_CHAINMAIL;
    case ITEM_CHESTPLATE_CHAINMAIL: return DURABILITY_CHEST_CHAINMAIL;
    case ITEM_LEGGINGS_CHAINMAIL:   return DURABILITY_LEGS_CHAINMAIL;
    case ITEM_BOOTS_CHAINMAIL:      return DURABILITY_BOOTS_CHAINMAIL;

    // Armor - Iron
    case ITEM_HELMET_IRON:      return DURABILITY_HELMET_IRON;
    case ITEM_CHESTPLATE_IRON:  return DURABILITY_CHEST_IRON;
    case ITEM_LEGGINGS_IRON:    return DURABILITY_LEGS_IRON;
    case ITEM_BOOTS_IRON:       return DURABILITY_BOOTS_IRON;

    // Armor - Diamond
    case ITEM_HELMET_DIAMOND:   return DURABILITY_HELMET_DIAMOND;
    case ITEM_CHESTPLATE_DIAMOND: return DURABILITY_CHEST_DIAMOND;
    case ITEM_LEGGINGS_DIAMOND: return DURABILITY_LEGS_DIAMOND;
    case ITEM_BOOTS_DIAMOND:    return DURABILITY_BOOTS_DIAMOND;

    // Armor - Gold
    case ITEM_HELMET_GOLD:      return DURABILITY_HELMET_GOLD;
    case ITEM_CHESTPLATE_GOLD:  return DURABILITY_CHEST_GOLD;
    case ITEM_LEGGINGS_GOLD:    return DURABILITY_LEGS_GOLD;
    case ITEM_BOOTS_GOLD:       return DURABILITY_BOOTS_GOLD;

    // Misc damageable
    case ITEM_FLINT_AND_STEEL:  return DURABILITY_FLINT_AND_STEEL;
    case ITEM_FISHING_ROD:      return DURABILITY_FISHING_ROD;
    case ITEM_SHEARS:           return DURABILITY_SHEARS;
    case ITEM_BOW:              return DURABILITY_BOW;

    default:
        return 0; // not damageable
    }
}