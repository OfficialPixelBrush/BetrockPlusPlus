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
#include "player_conn/player_session.h"
#include <cstdint>
#include <unordered_map>

struct WorldManager;
struct PlayerSession;
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

ItemDamage GetMaterialUses(ToolMaterial _material);

struct ToolProperties {
	ToolType type = ToolType::None;
	ToolMaterial material = ToolMaterial::None;
	ItemDamage maxUses = -1;
	TickTime predictedBreakTick = 0;
};

struct ItemProperties {
	ItemAmount maxStack = STACK_MAX;
};

struct ItemBehavior {
	void (*onBlockStartMining)(WorldManager& _world, ItemStack* _stack, Int3 _pos,
	                           PacketData::FaceDirection _face) = nullptr;
	void (*onBlockFinishMining)(WorldManager& _world, ItemStack* _stack, Int3 _pos,
	                            PacketData::FaceDirection _face) = nullptr;
	void (*onBlockUse)(WorldManager& _world, ItemStack* _stack, Int3 _pos, PacketData::FaceDirection _face) = nullptr;
	void (*onEntityAttack)(Entity& _attackedEntity, ItemStack* _stack) = nullptr;
	void (*onEntityUse)(Entity& _usedEntity, ItemStack* _stack) = nullptr;
	void (*onStartHolding)(ItemStack* _stack) = nullptr;
	void (*whileHeld)(ItemStack* _stack, PlayerSession& _session) = nullptr;
	void (*onStopHolding)(ItemStack* _stack) = nullptr;
};

extern std::unordered_map<ItemId, ItemBehavior> itemBehavior;
extern std::unordered_map<ItemId, ItemProperties> itemProperties;
extern std::unordered_map<ItemId, ToolProperties> toolProperties;
void RegisterAll();

bool IsValid(ItemId _id);

bool IsArmor(ItemId _id);
bool IsHoe(ItemId _id);
bool IsSword(ItemId _id);
bool IsPickaxe(ItemId _id);
bool IsAxe(ItemId _id);
bool IsShovel(ItemId _id);
bool IsWeapon(ItemId _id);
bool IsTool(ItemId _id);
bool IsThrowable(ItemId _id);
bool IsEdible(ItemId _id);
bool IsStackable(ItemId _id); // max stack > 1
bool IsBlock(ItemId _id);

void HarmTool(ItemStack* _stack);

// Returns max stack size for this item/block id
int32_t GetMaxStack(ItemId _id);

// Returns max durability (0 = not damageable)
ItemDamage GetMaxDurability(ItemId _id);
}; // namespace Items