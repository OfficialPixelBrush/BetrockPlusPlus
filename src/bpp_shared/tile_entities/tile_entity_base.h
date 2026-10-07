/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "inventory/inventories.h"
#include "nbt/nbt.h"
#include "numeric_structs.h"

enum class TileType : uint8_t {
	CHEST,
	FURNACE,
	DISPENSER,
	SIGN,
	SPAWNER,
	NOTEBLOCK,
	JUKEBOX,
	PISTON_MOVING
};

// I hate doing inheritance but its simple to do for this
struct Chunk;
class WorldManager;
struct TileEntity {
	TileType type;
	Int3 position{ 0, 0, 0 }; // Global coordinates
	bool canTick = false;
	Chunk* chunk =
	    nullptr; // The chunk this tile entity is in; may not be best practice to have this as a raw pointer but it should be fine since the chunk will always exist while the tile entity exists

	TileEntity(TileType _pType, Int3 _pPosition) : type(_pType), position(_pPosition) {};

	virtual void Tick(WorldManager& _world);
	virtual Tag Serialize();
	virtual ~TileEntity() = default;
};

static void SerializeInventory(Tag& _root, Inventory& _inventory) {
	auto items = Tag{ .type = TAG_LIST, .name = "Items", .longValue = 0, .listType = TAG_COMPOUND };
	int8_t currentSlot = 0;
	for (auto& stack : _inventory.slots) {
		if (stack.id != Items::Id::INVALID) {
			auto item = Tag{ .type = TAG_COMPOUND, .name = "", .longValue = 0 };
			auto count = Tag{ .type = TAG_BYTE, .name = "Count", .byteValue = stack.count };
			auto damage = Tag{ .type = TAG_SHORT, .name = "Damage", .shortValue = stack.data };
			auto id = Tag{ .type = TAG_SHORT, .name = "id", .shortValue = stack.id };
			auto slot = Tag{ .type = TAG_BYTE, .name = "Slot", .byteValue = currentSlot };

			item.compound["Count"] = count;
			item.compound["Damage"] = damage;
			item.compound["id"] = id;
			item.compound["Slot"] = slot;

			items.list.push_back(item);
		}
		currentSlot++;
	}

	_root.compound["Items"] = items;
}