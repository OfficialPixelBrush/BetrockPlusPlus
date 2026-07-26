/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "tile_entity.h"
#include "blocks.h"
#include "inventory/item_stack.h"
#include "items.h"
#include "items/item_properties.h"
#include "world/chunk.h"
#include <string>

//TODO: Move all these into seperate files just like InventoryInteraction

void TileEntityChest::Tick() {
	if (chunk && inventory.isModified) {
		chunk->isModified = true;
		inventory.isModified = false;
	}
}

static ItemStack GetSmeltingResult(ItemId& _input) {
	switch (_input) {
	case BLOCK_ORE_IRON:
		return { Items::IRON, 1, 0 };
	case BLOCK_ORE_GOLD:
		return { Items::GOLD, 1, 0 };
	case BLOCK_ORE_DIAMOND:
		return { Items::DIAMOND, 1, 0 };
	case BLOCK_SAND:
		return { BLOCK_GLASS, 1, 0 };
	case Items::PORKCHOP:
		return { Items::PORKCHOP_COOKED, 1, 0 };
	case Items::FISH:
		return { Items::FISH_COOKED, 1, 0 };
	case BLOCK_COBBLESTONE:
		return { BLOCK_STONE, 1, 0 };
	case Items::CLAY:
		return { Items::BRICK, 1, 0 };
	case BLOCK_CACTUS:
		return { Items::DYE, 1, 2 }; // Green dye
	}

	if (Items::IsBlock(_input) && Blocks::blockProperties[_input].material == Material::Wood())
		return { Items::COAL, 1, 1 }; // Charcoal

	return ItemStack{};
}

void TileEntityFurnace::Tick() {
	dirtyFlags = FLAG_NONE;

	const int maxBurnTime = GetMaxBurnTime();
	if (maxBurnTime != lastMaxBurnTime && maxBurnTime != 0) {
		dirtyFlags |= FLAG_MAX_BURN_TIME;
		lastMaxBurnTime = maxBurnTime;
	}

	ItemStack result = GetSmeltingResult(inventory.slots[0].id);
	{
		const int oldBurnTime = burnTime;
		if (--burnTime < 0) {
			if (result.id != Items::INVALID) {
				inventory.slots[1].DecrementCount(1);
				burnTime = inventory.slots[1].count > 0 ? maxBurnTime : 0;
			} else {
				burnTime = 0;
			}
		}

		if (burnTime != oldBurnTime)
			dirtyFlags |= FLAG_BURN_TIME;
	}

	{
		const int oldCookTime = cookTime;

		if (burnTime > 0) {
			if (result.id != Items::INVALID && ++cookTime >= 200) {
				if (inventory.MergeItemStackInInventory(result, false, 2, 2)) {
					inventory.slots[0].DecrementCount(1);
					cookTime = 0;
				} else {
					cookTime = 200;
				}
			}
		} else if (cookTime > 0) {
			--cookTime;
		}

		if (cookTime != oldCookTime)
			dirtyFlags |= FLAG_COOK_TIME;
	}

	if (chunk && inventory.isModified) {
		chunk->isModified = true;
		inventory.isModified = false;
	}
}

int TileEntityFurnace::GetMaxBurnTime() const {
	const ItemId fuel = inventory.slots[1].id;

	switch (fuel) {
	case Items::COAL:
		return 1600;
	case Items::STICK:
	case BLOCK_SAPLING:
		return 100;
	case Items::BUCKET_LAVA:
		return 20000;
	}

	if (Items::IsBlock(fuel) && Blocks::blockProperties[fuel].material == Material::Wood()) {
		return 300;
	}

	return 0;
}

int TileEntityFurnace::GetBurnTime() const {
	return burnTime;
}

int TileEntityFurnace::GetCookTime() const {
	return cookTime;
}

void TileEntityDispenser::Tick() {
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

Tag TileEntity::Serialize() {
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

Tag TileEntityChest::Serialize() {
	auto root = TileEntity::Serialize();

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

Tag TileEntityFurnace::Serialize() {
	auto root = TileEntity::Serialize();

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

Tag TileEntityDispenser::Serialize() {
	auto root = TileEntity::Serialize();

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

Tag TileEntitySign::Serialize() {
	auto root = TileEntity::Serialize();

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

Tag TileEntityMobSpawner::Serialize() {
	auto root = TileEntity::Serialize();

	auto vEntityId = Tag{ .type = TAG_STRING, .name = "EntityId", .stringValue = entityId };
	auto vDelay = Tag{ .type = TAG_SHORT, .name = "Delay", .shortValue = delay };

	root.compound["EntityId"] = vEntityId;
	root.compound["Delay"] = vDelay;

	return root;
}