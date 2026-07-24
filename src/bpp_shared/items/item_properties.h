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
struct PlayerSession;
struct Entity;
class Server;

namespace Items {

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
	void (*onStartHolding)(ItemStack* _stack, PlayerSession& _session) = nullptr;
	void (*whileHeld)(ItemStack* _stack, PlayerSession& _session, Server& _server) = nullptr;
	void (*onStopHolding)(ItemStack* _stack) = nullptr;
};

extern std::unordered_map<ItemId, ItemBehavior> itemBehavior;
extern std::unordered_map<ItemId, ItemProperties> itemProperties;
void RegisterAll();

void HarmTool(ItemStack* _stack);

// Returns max stack size for this item/block id
int32_t GetMaxStack(ItemId _id);

// Returns max durability (0 = not damageable)
ItemDamage GetMaxDurability(ItemId _id);

constexpr bool IsItem(ItemId _id) {
	return (_id >= Id::SHOVEL_IRON && _id < Id::MAX) || (_id >= Id::RECORD_13 && _id < Id::RECORD_MAX);
}

constexpr bool IsBlock(ItemId _id) {
	return _id > BLOCK_INVALID && _id < BLOCK_MAX;
}

constexpr bool IsValidId(ItemId _id) {
	return IsBlock(_id) || IsItem(_id);
}

constexpr bool IsArmor(ItemId _id) {
	return (_id >= Items::HELMET_LEATHER && _id <= Items::BOOTS_GOLD);
}

constexpr bool IsThrowable(ItemId _id) {
	return (_id == Items::SNOWBALL || _id == Items::EGG);
}

constexpr bool IsStackable(ItemId _id) {
	return Items::GetMaxStack(_id) > 1;
}
}; // namespace Items