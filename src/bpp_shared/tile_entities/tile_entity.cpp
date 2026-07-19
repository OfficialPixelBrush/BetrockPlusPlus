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
	if (m_chunk && m_inventory.m_isModified) {
		m_chunk->m_isModified = true;
		m_inventory.m_isModified = false;
	}
}

void TileEntityFurnace::tick() {
	if (m_chunk && m_inventory.m_isModified) {
		m_chunk->m_isModified = true;
		m_inventory.m_isModified = false;
	}
}

void TileEntityDispenser::tick() {
	if (m_chunk && m_inventory.m_isModified) {
		m_chunk->m_isModified = true;
		m_inventory.m_isModified = false;
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
	root.m_type = TAG_COMPOUND;

	auto id = Tag{ .m_type = TAG_STRING, .m_name = "id", .m_stringValue = getTileNbtId(m_type) };
	auto x = Tag{ .m_type = TAG_INT, .m_name = "x", .m_intValue = m_position.m_x };
	auto y = Tag{ .m_type = TAG_INT, .m_name = "y", .m_intValue = m_position.m_y };
	auto z = Tag{ .m_type = TAG_INT, .m_name = "z", .m_intValue = m_position.m_z };

	root.m_compound["id"] = id;
	root.m_compound["x"] = x;
	root.m_compound["y"] = y;
	root.m_compound["z"] = z;

	return root;
}

Tag TileEntityChest::serialize() {
	auto root = TileEntity::serialize();

	// Construct our inventory
	auto items = Tag{ .m_type = TAG_LIST, .m_name = "Items", .m_listType = TAG_COMPOUND };
	int8_t currentSlot = 0;
	for (auto& stack : m_inventory.m_slots) {
		if (stack.m_id != Items::Id::INVALID) {
			auto item = Tag{ .m_type = TAG_COMPOUND };
			auto Count = Tag{ .m_type = TAG_BYTE, .m_name = "Count", .m_byteValue = stack.m_count };
			auto Damage = Tag{ .m_type = TAG_SHORT, .m_name = "Damage", .m_shortValue = stack.m_data };
			auto Id = Tag{ .m_type = TAG_SHORT, .m_name = "id", .m_shortValue = stack.m_id };
			auto Slot = Tag{ .m_type = TAG_BYTE, .m_name = "Slot", .m_byteValue = currentSlot };

			item.m_compound["Count"] = Count;
			item.m_compound["Damage"] = Damage;
			item.m_compound["id"] = Id;
			item.m_compound["Slot"] = Slot;

			items.m_list.push_back(item);
		}
		currentSlot++;
	}

	root.m_compound["Items"] = items;

	return root;
}

Tag TileEntityFurnace::serialize() {
	auto root = TileEntity::serialize();

	// Construct our inventory
	auto items = Tag{ .m_type = TAG_LIST, .m_name = "Items", .m_listType = TAG_COMPOUND };
	int8_t currentSlot = 0;
	for (auto& stack : m_inventory.m_slots) {
		if (stack.m_id != Items::Id::INVALID) {
			auto item = Tag{ .m_type = TAG_COMPOUND };
			auto Count = Tag{ .m_type = TAG_BYTE, .m_name = "Count", .m_byteValue = stack.m_count };
			auto Damage = Tag{ .m_type = TAG_SHORT, .m_name = "Damage", .m_shortValue = stack.m_data };
			auto Id = Tag{ .m_type = TAG_SHORT, .m_name = "id", .m_shortValue = stack.m_id };
			auto Slot = Tag{ .m_type = TAG_BYTE, .m_name = "Slot", .m_byteValue = currentSlot };

			item.m_compound["Count"] = Count;
			item.m_compound["Damage"] = Damage;
			item.m_compound["id"] = Id;
			item.m_compound["Slot"] = Slot;

			items.m_list.push_back(item);
		}
		currentSlot++;
	}

	root.m_compound["Items"] = items;

	return root;
}

Tag TileEntityDispenser::serialize() {
	auto root = TileEntity::serialize();

	// Construct our inventory
	auto items = Tag{ .m_type = TAG_LIST, .m_name = "Items", .m_listType = TAG_COMPOUND };
	int8_t currentSlot = 0;
	for (auto& stack : m_inventory.m_slots) {
		if (stack.m_id != Items::Id::INVALID) {
			auto item = Tag{ .m_type = TAG_COMPOUND };
			auto Count = Tag{ .m_type = TAG_BYTE, .m_name = "Count", .m_byteValue = stack.m_count };
			auto Damage = Tag{ .m_type = TAG_SHORT, .m_name = "Damage", .m_shortValue = stack.m_data };
			auto Id = Tag{ .m_type = TAG_SHORT, .m_name = "id", .m_shortValue = stack.m_id };
			auto Slot = Tag{ .m_type = TAG_BYTE, .m_name = "Slot", .m_byteValue = currentSlot };

			item.m_compound["Count"] = Count;
			item.m_compound["Damage"] = Damage;
			item.m_compound["id"] = Id;
			item.m_compound["Slot"] = Slot;

			items.m_list.push_back(item);
		}
		currentSlot++;
	}

	root.m_compound["Items"] = items;

	return root;
}

Tag TileEntitySign::serialize() {
	auto root = TileEntity::serialize();

	auto text1 = Tag{ .m_type = TAG_STRING, .m_name = "Text1", .m_stringValue = m_text1 };
	auto text2 = Tag{ .m_type = TAG_STRING, .m_name = "Text2", .m_stringValue = m_text2 };
	auto text3 = Tag{ .m_type = TAG_STRING, .m_name = "Text3", .m_stringValue = m_text3 };
	auto text4 = Tag{ .m_type = TAG_STRING, .m_name = "Text4", .m_stringValue = m_text4 };

	root.m_compound["Text1"] = text1;
	root.m_compound["Text2"] = text2;
	root.m_compound["Text3"] = text3;
	root.m_compound["Text4"] = text4;

	return root;
}

Tag TileEntityMobSpawner::serialize() {
	auto root = TileEntity::serialize();

	auto entityId = Tag{ .m_type = TAG_STRING, .m_name = "EntityId", .m_stringValue = m_entityId };
	auto delay = Tag{ .m_type = TAG_SHORT, .m_name = "Delay", .m_shortValue = m_delay };

	root.m_compound["EntityId"] = entityId;
	root.m_compound["Delay"] = delay;

	return root;
}