/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "crafting.h"
#include "inventory/inventory_interaction.h"
#include "numeric_structs.h"

CraftingInventoryInteraction::CraftingInventoryInteraction(Inventory* _sharedInventory, Inventory* _craftingInventory,
                                                           InventoryPlayer* _playerInventory, Runtime& _gameRuntime,
                                                           UInt8_2 _gridSize)
    : InventoryInteraction(_sharedInventory), craftInventory(_craftingInventory), playerInventory(_playerInventory),
      runtime(_gameRuntime), gridSize(_gridSize) {}

void CraftingInventoryInteraction::updateResult() {
	auto grid = std::span<const ItemStack>(craftInventory->slots.data() + 1, gridSize.total());
	ItemStack result = runtime.recipeManager.matchGrid(grid, gridSize);
	craftInventory->slots[0] = result;
}

void CraftingInventoryInteraction::finishCraft() {
	craftInventory->slots[0] = ItemStack{};

	// Consume one of each ingredient that went into this craft
	for (size_t i = 1; i <= gridSize.total(); ++i)
		craftInventory->slots[i].decrementCount(1);

	// The grid changed, there might be another possible craft
	updateResult();
}

void CraftingInventoryInteraction::takeResult() {
	ItemStack& result = craftInventory->slots[0];
	if (result.id == Items::Id::INVALID)
		return;

	if (carried.id == Items::Id::INVALID) {
		carried = result;
	} else if (carried.id == result.id && carried.data == result.data) {
		// Same type, try merge with cursor
		int maxStack = Items::GetMaxStack(carried.id);
		if (int(carried.count) + int(result.count) > maxStack)
			return;
		carried.count += result.count;
	} else {
		// Cursor holds something else
		return;
	}

	finishCraft();
}

void CraftingInventoryInteraction::handleCrafting(int _slot) {
	if (_slot == 0 || _slot > gridSize.total())
		return;

	updateResult();
}

void CraftingInventoryInteraction::onLeftClick(int _slot) {
	if (_slot == 0)
		takeResult();
	else
		InventoryInteraction::onLeftClick(_slot);
	handleCrafting(_slot);
}

void CraftingInventoryInteraction::onRightClick(int _slot) {
	if (_slot == 0)
		takeResult();
	else
		InventoryInteraction::onRightClick(_slot);
	handleCrafting(_slot);
}

void CraftingInventoryInteraction::onShiftClick(int _slot) {
	if (_slot == 0)
		shiftClickResult();
	else
		shiftClickOther(_slot);
	handleCrafting(_slot);
}