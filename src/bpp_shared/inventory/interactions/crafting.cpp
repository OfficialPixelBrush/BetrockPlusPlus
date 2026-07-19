/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "crafting.h"
#include "inventory/inventory_interaction.h"
#include "numeric_structs.h"

CraftingInventoryInteraction::CraftingInventoryInteraction(Inventory* sharedInventory, Inventory* craftingInventory,
                                                           InventoryPlayer* playerInventory, Runtime& gameRuntime,
                                                           UInt8_2 gridSize)
    : InventoryInteraction(sharedInventory), m_craftInventory(craftingInventory), m_playerInventory(playerInventory),
      m_runtime(gameRuntime), m_gridSize(gridSize) {}

void CraftingInventoryInteraction::updateResult() {
	auto grid = std::span<const ItemStack>(m_craftInventory->m_slots.data() + 1, m_gridSize.total());
	ItemStack result = m_runtime.m_recipeManager.matchGrid(grid, m_gridSize);
	m_craftInventory->m_slots[0] = result;
}

void CraftingInventoryInteraction::finishCraft() {
	m_craftInventory->m_slots[0] = ItemStack{};

	// Consume one of each ingredient that went into this craft
	for (size_t i = 1; i <= m_gridSize.total(); ++i)
		m_craftInventory->m_slots[i].decrementCount(1);

	// The grid changed, there might be another possible craft
	updateResult();
}

void CraftingInventoryInteraction::takeResult() {
	ItemStack& result = m_craftInventory->m_slots[0];
	if (result.m_id == Items::Id::INVALID)
		return;

	if (m_carried.m_id == Items::Id::INVALID) {
		m_carried = result;
	} else if (m_carried.m_id == result.m_id && m_carried.m_data == result.m_data) {
		// Same type, try merge with cursor
		int maxStack = Items::GetMaxStack(m_carried.m_id);
		if (int(m_carried.m_count) + int(result.m_count) > maxStack)
			return;
		m_carried.m_count += result.m_count;
	} else {
		// Cursor holds something else
		return;
	}

	finishCraft();
}

void CraftingInventoryInteraction::handleCrafting(int slot) {
	if (slot == 0 || slot > m_gridSize.total())
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