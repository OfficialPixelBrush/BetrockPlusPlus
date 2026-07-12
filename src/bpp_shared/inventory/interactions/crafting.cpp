/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "crafting.h"
#include "inventory/inventory_interaction.h"

CraftingInventoryInteraction::CraftingInventoryInteraction(Inventory* sharedInventory, Inventory* craftingInventory,
                                                           InventoryPlayer* playerInventory, Runtime& gameRuntime,
                                                           uint8_t width, uint8_t height)
    : InventoryInteraction(sharedInventory), craftInventory(craftingInventory), playerInventory(playerInventory),
      runtime(gameRuntime), gridWidth(width), gridHeight(height) {}

void CraftingInventoryInteraction::updateResult() {
	auto grid = std::span<const ItemStack>(craftInventory->slots.data() + 1, gridWidth * gridHeight);
	ItemStack result = runtime.recipeManager.matchGrid(grid, gridWidth, gridHeight);
	craftInventory->slots[0] = result;
}

void CraftingInventoryInteraction::finishCraft() {
	craftInventory->slots[0] = ItemStack{};

	// Consume one of each ingredient that went into this craft
	for (size_t i = 1; i <= gridWidth * gridHeight; ++i)
		craftInventory->slots[i].decrementCount(1);

	// The grid changed, there might be another possible craft
	updateResult();
}

void CraftingInventoryInteraction::takeResult() {
	ItemStack& result = craftInventory->slots[0];
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

void CraftingInventoryInteraction::handleCrafting(int slot) {
	if (slot == 0 || slot > gridWidth * gridHeight)
		return;

	updateResult();
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

void CraftingInventoryInteraction::onShiftClick(int slot) {
	if (slot == 0)
		shiftClickResult();
	else
		shiftClickOther(slot);
	handleCrafting(slot);
}