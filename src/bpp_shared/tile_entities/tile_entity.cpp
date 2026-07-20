/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "tile_entity.h"
#include "items.h"
#include "world/chunk.h"
#include <string>

void TileEntityChest::tick() {
	if (chunk && inventory.isModified) {
		chunk->isModified = true;
		inventory.isModified = false;
	}
}

void TileEntityFurnace::tick() {
	if (chunk && inventory.isModified) {
		chunk->isModified = true;
		inventory.isModified = false;
	}
}

void TileEntityDispenser::tick() {
	if (chunk && inventory.isModified) {
		chunk->isModified = true;
		inventory.isModified = false;
	}
}

constexpr std::string GetTileNbtId(TileType _type) {
	switch (_type) {
	case TileType::CHEST:
		return "Chest";
	case TileType::DISPENSER:
		return "Trap";
	case TileType::FURNACE:
		return "Furnace";
	case TileType::SIGN:
		return "Sign";
	case TileType::SPAWNER:
		return "MobSpawner";
	}
	// Invalid type, empty string
	return "";
}

Tag TileEntity::serialize() {
	auto root = Tag{};
	root.type = TAG_COMPOUND;

	auto id = Tag{ .type = TAG_STRING, .name = "id", .stringValue = GetTileNbtId(type) };
	auto x = Tag{ .type = TAG_INT, .name = "x", .intValue = position.x };
	auto y = Tag{ .type = TAG_INT, .name = "y", .intValue = position.y };
	auto z = Tag{ .type = TAG_INT, .name = "z", .intValue = position.z };

	root.compound["id"] = id;
	root.compound["x"] = x;
	root.compound["y"] = y;
	root.compound["z"] = z;

	return root;
}

Tag TileEntityChest::serialize() {
	auto root = TileEntity::serialize();

	// Construct our inventory
	auto items = Tag{ .type = TAG_LIST, .name = "Items", .listType = TAG_COMPOUND };
	int8_t currentSlot = 0;
	for (auto& stack : inventory.slots) {
		if (stack.id != Items::Id::INVALID) {
			auto item = Tag{ .type = TAG_COMPOUND };
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

	root.compound["Items"] = items;

	return root;
}

Tag TileEntityFurnace::serialize() {
	auto root = TileEntity::serialize();

	// Construct our inventory
	auto items = Tag{ .type = TAG_LIST, .name = "Items", .listType = TAG_COMPOUND };
	int8_t currentSlot = 0;
	for (auto& stack : inventory.slots) {
		if (stack.id != Items::Id::INVALID) {
			auto item = Tag{ .type = TAG_COMPOUND };
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

	root.compound["Items"] = items;

	return root;
}

Tag TileEntityDispenser::serialize() {
	auto root = TileEntity::serialize();

	// Construct our inventory
	auto items = Tag{ .type = TAG_LIST, .name = "Items", .listType = TAG_COMPOUND };
	int8_t currentSlot = 0;
	for (auto& stack : inventory.slots) {
		if (stack.id != Items::Id::INVALID) {
			auto item = Tag{ .type = TAG_COMPOUND };
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

	root.compound["Items"] = items;

	return root;
}

Tag TileEntitySign::serialize() {
	auto root = TileEntity::serialize();

	auto vText1 = Tag{ .type = TAG_STRING, .name = "Text1", .stringValue = text1 };
	auto vText2 = Tag{ .type = TAG_STRING, .name = "Text2", .stringValue = text2 };
	auto vText3 = Tag{ .type = TAG_STRING, .name = "Text3", .stringValue = text3 };
	auto vText4 = Tag{ .type = TAG_STRING, .name = "Text4", .stringValue = text4 };

	root.compound["Text1"] = vText1;
	root.compound["Text2"] = vText2;
	root.compound["Text3"] = vText3;
	root.compound["Text4"] = vText4;

	return root;
}

Tag TileEntityMobSpawner::serialize() {
	auto root = TileEntity::serialize();

	auto vEntityId = Tag{ .type = TAG_STRING, .name = "EntityId", .stringValue = entityId };
	auto vDelay = Tag{ .type = TAG_SHORT, .name = "Delay", .shortValue = delay };

	root.compound["EntityId"] = vEntityId;
	root.compound["Delay"] = vDelay;

	return root;
}