/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "base_types.h"
#include "items.h"
#include "numeric_structs.h"
#include "packet_data.h"
#include <cstdint>
#include <unordered_map>

struct WorldManager;
struct Entity;

namespace Items {

enum class ToolLevel : int8_t {
	None = -1,
	WoodenOrGold = 0,
	Stone = 1,
	Iron = 2,
	Diamond = 3
};
enum class ToolMaterial : int8_t {
	None = -1,
	Wooden = 0,
	Gold = 1,
	Stone = 2,
	Iron = 3,
	Diamond = 4
};
enum class ToolType : int8_t {
	None = -1,
	Hoe = 0,
	Shovel = 1,
	Pickaxe = 2,
	Axe = 3,
	Sword = 4,
};
enum class ArmorType : int8_t {
	None = -1,
	Boots = 0,
	Leggings = 1,
	Chestplate = 2,
	Helmet = 3,
};

ItemDamage GetMaterialUses(ToolMaterial material);

struct ToolProperties {
	ToolType m_type = ToolType::None;
	ToolMaterial m_material = ToolMaterial::None;
	ItemDamage m_max_uses = -1;
};

struct ItemProperties {
	ItemAmount m_max_stack = STACK_MAX;
};

struct ItemBehavior {
	void (*m_onBlockStartMining)(WorldManager& world, ItemStack* stack, Int3 pos,
	                           PacketData::FaceDirection face) = nullptr;
	void (*m_onBlockFinishMining)(WorldManager& world, ItemStack* stack, Int3 pos,
	                            PacketData::FaceDirection face) = nullptr;
	void (*m_onBlockUse)(WorldManager& world, ItemStack* stack, Int3 pos, PacketData::FaceDirection face) = nullptr;
	void (*m_onEntityAttack)(Entity& attackedEntity, ItemStack* stack) = nullptr;
	void (*m_onEntityUse)(Entity& usedEntity, ItemStack* stack) = nullptr;
};

extern std::unordered_map<ItemId, ItemBehavior> itemBehavior;
extern std::unordered_map<ItemId, ItemProperties> itemProperties;
extern std::unordered_map<ItemId, ToolProperties> toolProperties;
void registerAll();

bool IsValid(ItemId id);

bool IsArmor(ItemId id);
bool IsHoe(ItemId id);
bool IsSword(ItemId id);
bool IsPickaxe(ItemId id);
bool IsAxe(ItemId id);
bool IsShovel(ItemId id);
bool IsWeapon(ItemId id);
bool IsTool(ItemId id);
bool IsThrowable(ItemId id);
bool IsEdible(ItemId id);
bool IsStackable(ItemId id); // max stack > 1
bool IsBlock(ItemId id);

void harmTool(ItemStack* stack);

// Returns max stack size for this item/block id
int32_t GetMaxStack(ItemId id);

// Returns max durability (0 = not damageable)
ItemDamage GetMaxDurability(ItemId id);
}; // namespace Items