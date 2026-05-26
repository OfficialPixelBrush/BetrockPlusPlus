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
	std::string id = "";
	Int3 position{ 0, 0, 0 }; // Global coordinates
	bool canTick = false;

	TileEntity(std::string pId, Int3 pPosition) : id(pId), position(pPosition) {};

	virtual void tick() {};
	virtual Tag serialize() {
		auto root = Tag{};
		root.type = TAG_COMPOUND;

		auto id = Tag{ .type = TAG_STRING, .name = "id", .stringValue = this->id };
		auto x = Tag{ .type = TAG_INT,    .name = "x",  .intValue = this->position.x };
		auto y = Tag{ .type = TAG_INT,    .name = "y",  .intValue = this->position.y };
		auto z = Tag{ .type = TAG_INT,    .name = "z",  .intValue = this->position.z };

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

		auto id = Tag{ .type = TAG_STRING, .name = "id", .stringValue = this->id };
		auto x = Tag{ .type = TAG_INT,    .name = "x",  .intValue = this->position.x };
		auto y = Tag{ .type = TAG_INT,    .name = "y",  .intValue = this->position.y };
		auto z = Tag{ .type = TAG_INT,    .name = "z",  .intValue = this->position.z };

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
	TileEntityFurnace(Int3 pPosition) : TileEntity("Furnace", pPosition) { canTick = true; };

	Tag serialize() override {
		auto root = Tag{};
		root.type = TAG_COMPOUND;

		auto id = Tag{ .type = TAG_STRING, .name = "id", .stringValue = this->id };
		auto x = Tag{ .type = TAG_INT,    .name = "x",  .intValue = this->position.x };
		auto y = Tag{ .type = TAG_INT,    .name = "y",  .intValue = this->position.y };
		auto z = Tag{ .type = TAG_INT,    .name = "z",  .intValue = this->position.z };

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

		auto id = Tag{ .type = TAG_STRING, .name = "id", .stringValue = this->id };
		auto x = Tag{ .type = TAG_INT,    .name = "x",  .intValue = this->position.x };
		auto y = Tag{ .type = TAG_INT,    .name = "y",  .intValue = this->position.y };
		auto z = Tag{ .type = TAG_INT,    .name = "z",  .intValue = this->position.z };

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
	std::string Text1 = "";
	std::string Text2 = "";
	std::string Text3 = "";
	std::string Text4 = "";
	TileEntitySign(Int3 pPosition) : TileEntity("Sign", pPosition) {};

	Tag serialize() override {
		auto root = Tag{};
		root.type = TAG_COMPOUND;

		auto id = Tag{ .type = TAG_STRING, .name = "id",    .stringValue = this->id };
		auto x = Tag{ .type = TAG_INT,    .name = "x",     .intValue = this->position.x };
		auto y = Tag{ .type = TAG_INT,    .name = "y",     .intValue = this->position.y };
		auto z = Tag{ .type = TAG_INT,    .name = "z",     .intValue = this->position.z };
		auto Text1 = Tag{ .type = TAG_STRING, .name = "Text1", .stringValue = this->Text1 };
		auto Text2 = Tag{ .type = TAG_STRING, .name = "Text2", .stringValue = this->Text2 };
		auto Text3 = Tag{ .type = TAG_STRING, .name = "Text3", .stringValue = this->Text3 };
		auto Text4 = Tag{ .type = TAG_STRING, .name = "Text4", .stringValue = this->Text4 };

		root.compound["Text1"] = Text1;
		root.compound["Text2"] = Text2;
		root.compound["Text3"] = Text3;
		root.compound["Text4"] = Text4;
		root.compound["id"] = id;
		root.compound["x"] = x;
		root.compound["y"] = y;
		root.compound["z"] = z;

		return root;
	};
};

// MobSpawner
struct TileEntityMobSpawner : TileEntity {
	std::string EntityId = "";
	int16_t delay = 0;
	TileEntityMobSpawner(Int3 pPosition) : TileEntity("MobSpawner", pPosition) { canTick = true; };

	Tag serialize() override {
		auto root = Tag{};
		root.type = TAG_COMPOUND;

		auto id = Tag{ .type = TAG_STRING, .name = "id",       .stringValue = this->id };
		auto x = Tag{ .type = TAG_INT,    .name = "x",        .intValue = this->position.x };
		auto y = Tag{ .type = TAG_INT,    .name = "y",        .intValue = this->position.y };
		auto z = Tag{ .type = TAG_INT,    .name = "z",        .intValue = this->position.z };
		auto EntityId = Tag{ .type = TAG_STRING, .name = "EntityId", .stringValue = this->EntityId };
		auto Delay = Tag{ .type = TAG_SHORT,  .name = "Delay",    .shortValue = this->delay };

		root.compound["EntityId"] = EntityId;
		root.compound["Delay"] = Delay;
		root.compound["id"] = id;
		root.compound["x"] = x;
		root.compound["y"] = y;
		root.compound["z"] = z;

		return root;
	};
};