/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "crafting.h"
#include "blocks.h"
#include "inventory/item_stack.h"
#include "items.h"

CraftingInventoryInteraction::CraftingInventoryInteraction(InventoryPlayer* pinv, WorldManager& worldMng,
                                                           Runtime& gameRuntime, Int3 craftingTablePos)
    : InventoryInteraction(&sharedInventory), playerInventory(pinv), world(worldMng), runtime(gameRuntime),
      blockPosition(craftingTablePos) {
	sharedInventory.owner = this;
	mergeInventories();
}

CraftingInventoryInteraction::~CraftingInventoryInteraction() {
	writeBack();

	// Return the crafting grid back to the player
	for (size_t i = 1; i < 10; i++) {
		ItemStack& stack = craftInventory.slots[i];
		if (stack.id == ITEM_INVALID)
			continue;
		playerInventory->mergeItemStackInInventory(stack, true, 9, 44);
	}
}

bool CraftingInventoryInteraction::canExist() {
	return world.getBlockId(blockPosition) == BLOCK_CRAFTING_TABLE;
}

void CraftingInventoryInteraction::initSnapshot() {
	snapshot.resize(size_t(craftInventory.getSizeInventory()));
	for (size_t i = 0; i < size_t(craftInventory.getSizeInventory()); i++)
		snapshot[i] = craftInventory.slots[i];
}

// Analyze the snapshot vs the current crafting table inventory
std::vector<DeltaSlot> CraftingInventoryInteraction::tickDiff() {
	std::vector<DeltaSlot> differences;
	for (size_t i = 0; i < snapshot.size(); i++) {
		auto& snap = snapshot[i];

		bool changed = snap != craftInventory.slots[i];
		if (!changed)
			continue;

		snap = craftInventory.slots[i];
		differences.push_back({ snap, int(i) });
	}
	mergeInventories();
	return differences;
}

void CraftingInventoryInteraction::mergeInventories() {
	size_t slotCount = 0;
	for (size_t i = 0; i < 10; i++)
		sharedInventory.slots[slotCount++] = craftInventory.slots[i];
	for (size_t i = 9; i < 44; i++)
		sharedInventory.slots[slotCount++] = playerInventory->slots[i];
}

void CraftingInventoryInteraction::writeBack() {
	size_t slotCount = 0;
	for (size_t i = 0; i < 10; i++)
		craftInventory.slots[i] = sharedInventory.slots[slotCount++];
	for (size_t i = 9; i < 44; i++)
		playerInventory->slots[i] = sharedInventory.slots[slotCount++];
}

void CraftingInventoryInteraction::updateResult() {
	auto grid = std::span<const ItemStack, 9>(craftInventory.slots.data() + 1, 9);
	ItemStack result = runtime.recipeManager.matchGrid(grid);
	lastResult = result;
	craftInventory.slots[0] = result;
}

void CraftingInventoryInteraction::handleCrafting(int slot) {
	if (slot == 0 || slot > 9)
		return;

	// Grid changed (either by player or decrementCount)
	updateResult();
}

void CraftingInventoryInteraction::finishCraft() {
	craftInventory.slots[0] = ItemStack{};

	// Consume one of each ingredient that went into this craft
	for (size_t i = 1; i < 10; ++i)
		craftInventory.slots[i].decrementCount(1);

	// The grid changed, there might be another possible craft
	updateResult();
	mergeInventories();
}

void CraftingInventoryInteraction::takeResult() {
	ItemStack& result = craftInventory.slots[0];
	if (result.id == ITEM_INVALID)
		return;

	if (carried.id == ITEM_INVALID) {
		carried = result;
	} else if (carried.id == result.id && carried.data == result.data) {
		// Same type, try merge with cursor
		int maxStack = GetMaxStack(carried.id);
		if (int(carried.count) + int(result.count) > maxStack)
			return;
		carried.count += result.count;
	} else {
		// Cursor holds something else
		return;
	}

	finishCraft();
}

void CraftingInventoryInteraction::onLeftClick(int slot) {
	if (slot == 0)
		takeResult();
	else
		InventoryInteraction::onLeftClick(slot);
	handleCrafting(slot);
}

void CraftingInventoryInteraction::onRightClick(int slot) {
	if (slot == 0)
		takeResult();
	else
		InventoryInteraction::onRightClick(slot);
	handleCrafting(slot);
}

void CraftingInventoryInteraction::shiftClickResult() {
	ItemStack& result = craftInventory.slots[0];
	if (result.id == ITEM_INVALID)
		return;

	ItemStack copy = result;
	bool success = playerInventory->mergeItemStackInInventory(copy, true, 9, 44);
	if (success)
		finishCraft();
}

void CraftingInventoryInteraction::onShiftClick(int slot) {
	if (slot == 0)
		shiftClickResult();
	else
		shiftClickGrid(slot);
	handleCrafting(slot);
}

void CraftingInventoryInteraction::shiftClickGrid(int slot) {
	auto stack = sharedInventory.getStackInSlot(slot);
	if (!stack)
		return;

	ItemStack copy = *stack;

	if (slot < 10) {
		// Chest -> inventory
		[[maybe_unused]] bool success = playerInventory->mergeItemStackInInventory(copy, true, 9, 44);
	} else {
		// Inventory -> Chest
		[[maybe_unused]] bool success = craftInventory.mergeItemStackInInventory(copy, false, 1, 9);
	}

	// Update the source in the real inventory before re-merging
	if (slot < 10) {
		craftInventory.slots[slot] = copy.count == 0 ? ItemStack{} : copy;
	} else {
		playerInventory->slots[slot - 1] = copy.count == 0 ? ItemStack{} : copy;
	}

	// Re-sync sharedInventory from the real inventories
	mergeInventories();
}