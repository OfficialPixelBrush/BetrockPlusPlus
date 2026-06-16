/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "nbt/nbt.h"
#include "numeric_structs.h"
#include "inventory/inventories.h"

// I hate doing inheritance but its simple to do for this
struct TileEntity {
	std::string m_id = "";
	Int3 m_position{ 0, 0, 0 }; // Global coordinates
	bool m_canTick = false;

	TileEntity(std::string pId, Int3 pPosition) : m_id(pId), m_position(pPosition) {};

	virtual void tick() {};
	virtual Tag serialize() {
		auto root = Tag{};
		root.type = TAG_COMPOUND;

		auto id = Tag{ .type = TAG_STRING, .name = "id", .stringValue = m_id };
		auto x 	= Tag{ .type = TAG_INT,    .name = "x",  .intValue = m_position.x };
		auto y 	= Tag{ .type = TAG_INT,    .name = "y",  .intValue = m_position.y };
		auto z 	= Tag{ .type = TAG_INT,    .name = "z",  .intValue = m_position.z };

		root.compound["id"] = id;
		root.compound["x"] = x;
		root.compound["y"] = y;
		root.compound["z"] = z;

		return root;
	};
	virtual ~TileEntity() = default;
};

// Chest
struct TileEntityChest : TileEntity {
	InventoryChest inventory;
	TileEntityChest(Int3 pPosition) : TileEntity("Chest", pPosition) {};

	Tag serialize() override {
		auto root = Tag{};
		root.type = TAG_COMPOUND;

		auto id = Tag{ .type = TAG_STRING, .name = "id", .stringValue = m_id };
		auto x 	= Tag{ .type = TAG_INT,    .name = "x",  .intValue = m_position.x };
		auto y 	= Tag{ .type = TAG_INT,    .name = "y",  .intValue = m_position.y };
		auto z 	= Tag{ .type = TAG_INT,    .name = "z",  .intValue = m_position.z };

		// Construct our inventory
		auto items = Tag{ .type = TAG_LIST, .name = "Items", .listType = TAG_COMPOUND };
		int8_t currentSlot = 0;
		for (auto& stack : inventory.slots) {
			if (stack.has_value()) {
				auto item = Tag{ .type = TAG_COMPOUND };
				auto Count = Tag{ .type = TAG_BYTE,  .name = "Count",  .byteValue = stack->count };
				auto Damage = Tag{ .type = TAG_SHORT, .name = "Damage", .shortValue = stack->data };
				auto Id = Tag{ .type = TAG_SHORT, .name = "id",     .shortValue = stack->id };
				auto Slot = Tag{ .type = TAG_BYTE,  .name = "Slot",   .byteValue = currentSlot };

				item.compound["Count"] = Count;
				item.compound["Damage"] = Damage;
				item.compound["id"] = Id;
				item.compound["Slot"] = Slot;

				items.list.push_back(item);
			}
			currentSlot++;
		}

		root.compound["Items"] = items;
		root.compound["id"] = id;
		root.compound["x"] = x;
		root.compound["y"] = y;
		root.compound["z"] = z;

		return root;
	};
};

// Furnace
struct TileEntityFurnace : TileEntity {
	InventoryFurnace inventory;
	TileEntityFurnace(Int3 pPosition) : TileEntity("Furnace", pPosition) { m_canTick = true; };

	Tag serialize() override {
		auto root = Tag{};
		root.type = TAG_COMPOUND;

		auto id = Tag{ .type = TAG_STRING, .name = "id", .stringValue = m_id };
		auto x 	= Tag{ .type = TAG_INT,    .name = "x",  .intValue = m_position.x };
		auto y 	= Tag{ .type = TAG_INT,    .name = "y",  .intValue = m_position.y };
		auto z 	= Tag{ .type = TAG_INT,    .name = "z",  .intValue = m_position.z };

		// Construct our inventory
		auto items = Tag{ .type = TAG_LIST, .name = "Items", .listType = TAG_COMPOUND };
		int8_t currentSlot = 0;
		for (auto& stack : inventory.slots) {
			if (stack.has_value()) {
				auto item = Tag{ .type = TAG_COMPOUND };
				auto Count = Tag{ .type = TAG_BYTE,  .name = "Count",  .byteValue = stack->count };
				auto Damage = Tag{ .type = TAG_SHORT, .name = "Damage", .shortValue = stack->data };
				auto Id = Tag{ .type = TAG_SHORT, .name = "id",     .shortValue = stack->id };
				auto Slot = Tag{ .type = TAG_BYTE,  .name = "Slot",   .byteValue = currentSlot };

				item.compound["Count"] = Count;
				item.compound["Damage"] = Damage;
				item.compound["id"] = Id;
				item.compound["Slot"] = Slot;

				items.list.push_back(item);
			}
			currentSlot++;
		}

		root.compound["Items"] = items;
		root.compound["id"] = id;
		root.compound["x"] = x;
		root.compound["y"] = y;
		root.compound["z"] = z;

		return root;
	};
};

// Dispenser (Trap)
struct TileEntityDispenser : TileEntity {
	InventoryDispenser inventory;
	TileEntityDispenser(Int3 pPosition) : TileEntity("Trap", pPosition) {};

	Tag serialize() override {
		auto root = Tag{};
		root.type = TAG_COMPOUND;

		auto id = Tag{ .type = TAG_STRING, .name = "id", .stringValue = m_id };
		auto x 	= Tag{ .type = TAG_INT,    .name = "x",  .intValue = m_position.x };
		auto y 	= Tag{ .type = TAG_INT,    .name = "y",  .intValue = m_position.y };
		auto z 	= Tag{ .type = TAG_INT,    .name = "z",  .intValue = m_position.z };

		// Construct our inventory
		auto items = Tag{ .type = TAG_LIST, .name = "Items", .listType = TAG_COMPOUND };
		int8_t currentSlot = 0;
		for (auto& stack : inventory.slots) {
			if (stack.has_value()) {
				auto item = Tag{ .type = TAG_COMPOUND };
				auto Count = Tag{ .type = TAG_BYTE,  .name = "Count",  .byteValue = stack->count };
				auto Damage = Tag{ .type = TAG_SHORT, .name = "Damage", .shortValue = stack->data };
				auto Id = Tag{ .type = TAG_SHORT, .name = "id",     .shortValue = stack->id };
				auto Slot = Tag{ .type = TAG_BYTE,  .name = "Slot",   .byteValue = currentSlot };

				item.compound["Count"] = Count;
				item.compound["Damage"] = Damage;
				item.compound["id"] = Id;
				item.compound["Slot"] = Slot;

				items.list.push_back(item);
			}
			currentSlot++;
		}

		root.compound["Items"] = items;
		root.compound["id"] = id;
		root.compound["x"] = x;
		root.compound["y"] = y;
		root.compound["z"] = z;

		return root;
	};
};

// Sign
struct TileEntitySign : TileEntity {
	std::string m_text1 = "";
	std::string m_text2 = "";
	std::string m_text3 = "";
	std::string m_text4 = "";
	TileEntitySign(Int3 pPosition) : TileEntity("Sign", pPosition) {};

	Tag serialize() override {
		auto root = Tag{};
		root.type = TAG_COMPOUND;

		auto id = Tag{ .type = TAG_STRING, .name = "id",    .stringValue = m_id };
		auto x 	= Tag{ .type = TAG_INT,    .name = "x",     .intValue = m_position.x };
		auto y 	= Tag{ .type = TAG_INT,    .name = "y",     .intValue = m_position.y };
		auto z 	= Tag{ .type = TAG_INT,    .name = "z",     .intValue = m_position.z };
		auto text1 = Tag{ .type = TAG_STRING, .name = "Text1", .stringValue = m_text1 };
		auto text2 = Tag{ .type = TAG_STRING, .name = "Text2", .stringValue = m_text2 };
		auto text3 = Tag{ .type = TAG_STRING, .name = "Text3", .stringValue = m_text3 };
		auto text4 = Tag{ .type = TAG_STRING, .name = "Text4", .stringValue = m_text4 };

		root.compound["Text1"] = text1;
		root.compound["Text2"] = text2;
		root.compound["Text3"] = text3;
		root.compound["Text4"] = text4;
		root.compound["id"] = id;
		root.compound["x"] = x;
		root.compound["y"] = y;
		root.compound["z"] = z;

		return root;
	};
};

// MobSpawner
struct TileEntityMobSpawner : TileEntity {
	std::string m_entityId = "";
	int16_t m_delay = 0;
	TileEntityMobSpawner(Int3 pPosition) : TileEntity("MobSpawner", pPosition) { m_canTick = true; };

	Tag serialize() override {
		auto root = Tag{};
		root.type = TAG_COMPOUND;

		auto id = Tag{ .type = TAG_STRING, .name = "id",       .stringValue = m_id };
		auto x = Tag{ .type = TAG_INT,    .name = "x",        .intValue = m_position.x };
		auto y = Tag{ .type = TAG_INT,    .name = "y",        .intValue = m_position.y };
		auto z = Tag{ .type = TAG_INT,    .name = "z",        .intValue = m_position.z };
		auto entityId = Tag{ .type = TAG_STRING, .name = "EntityId", .stringValue = m_entityId };
		auto delay = Tag{ .type = TAG_SHORT,  .name = "Delay",    .shortValue = m_delay };

		root.compound["EntityId"] = entityId;
		root.compound["Delay"] = delay;
		root.compound["id"] = id;
		root.compound["x"] = x;
		root.compound["y"] = y;
		root.compound["z"] = z;

		return root;
	};
};