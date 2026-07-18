/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "tile_entity.h"
#include "items.h"
#include "world/chunk.h"

void TileEntityChest::tick() {
	if (m_chunk && inventory.isModified) {
		m_chunk->isModified = true;
		inventory.isModified = false;
	}
}

void TileEntityFurnace::tick() {
	if (m_chunk && inventory.isModified) {
		m_chunk->isModified = true;
		inventory.isModified = false;
	}
}

void TileEntityDispenser::tick() {
	if (m_chunk && inventory.isModified) {
		m_chunk->isModified = true;
		inventory.isModified = false;
	}
}

constexpr std::string getTileNbtId(TileType type) {
	switch (type) {
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
}

Tag TileEntity::serialize() {
	auto root = Tag{};
	root.type = TAG_COMPOUND;

	auto id = Tag{ .type = TAG_STRING, .name = "id", .stringValue = getTileNbtId(m_type) };
	auto x = Tag{ .type = TAG_INT, .name = "x", .intValue = m_position.x };
	auto y = Tag{ .type = TAG_INT, .name = "y", .intValue = m_position.y };
	auto z = Tag{ .type = TAG_INT, .name = "z", .intValue = m_position.z };

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
			auto Count = Tag{ .type = TAG_BYTE, .name = "Count", .byteValue = stack.count };
			auto Damage = Tag{ .type = TAG_SHORT, .name = "Damage", .shortValue = stack.data };
			auto Id = Tag{ .type = TAG_SHORT, .name = "id", .shortValue = stack.id };
			auto Slot = Tag{ .type = TAG_BYTE, .name = "Slot", .byteValue = currentSlot };

			item.compound["Count"] = Count;
			item.compound["Damage"] = Damage;
			item.compound["id"] = Id;
			item.compound["Slot"] = Slot;

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
			auto Count = Tag{ .type = TAG_BYTE, .name = "Count", .byteValue = stack.count };
			auto Damage = Tag{ .type = TAG_SHORT, .name = "Damage", .shortValue = stack.data };
			auto Id = Tag{ .type = TAG_SHORT, .name = "id", .shortValue = stack.id };
			auto Slot = Tag{ .type = TAG_BYTE, .name = "Slot", .byteValue = currentSlot };

			item.compound["Count"] = Count;
			item.compound["Damage"] = Damage;
			item.compound["id"] = Id;
			item.compound["Slot"] = Slot;

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
			auto Count = Tag{ .type = TAG_BYTE, .name = "Count", .byteValue = stack.count };
			auto Damage = Tag{ .type = TAG_SHORT, .name = "Damage", .shortValue = stack.data };
			auto Id = Tag{ .type = TAG_SHORT, .name = "id", .shortValue = stack.id };
			auto Slot = Tag{ .type = TAG_BYTE, .name = "Slot", .byteValue = currentSlot };

			item.compound["Count"] = Count;
			item.compound["Damage"] = Damage;
			item.compound["id"] = Id;
			item.compound["Slot"] = Slot;

			items.list.push_back(item);
		}
		currentSlot++;
	}

	root.compound["Items"] = items;

	return root;
}

Tag TileEntitySign::serialize() {
	auto root = TileEntity::serialize();

	auto text1 = Tag{ .type = TAG_STRING, .name = "Text1", .stringValue = m_text1 };
	auto text2 = Tag{ .type = TAG_STRING, .name = "Text2", .stringValue = m_text2 };
	auto text3 = Tag{ .type = TAG_STRING, .name = "Text3", .stringValue = m_text3 };
	auto text4 = Tag{ .type = TAG_STRING, .name = "Text4", .stringValue = m_text4 };

	root.compound["Text1"] = text1;
	root.compound["Text2"] = text2;
	root.compound["Text3"] = text3;
	root.compound["Text4"] = text4;

	return root;
}

Tag TileEntityMobSpawner::serialize() {
	auto root = TileEntity::serialize();

	auto entityId = Tag{ .type = TAG_STRING, .name = "EntityId", .stringValue = m_entityId };
	auto delay = Tag{ .type = TAG_SHORT, .name = "Delay", .shortValue = m_delay };

	root.compound["EntityId"] = entityId;
	root.compound["Delay"] = delay;

	return root;
}