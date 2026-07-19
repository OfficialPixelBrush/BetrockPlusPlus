/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "crafting_table.h"
#include "blocks.h"
#include "inventory/interactions/crafting.h"
#include "inventory/item_stack.h"
#include "items.h"

CraftingTableInventoryInteraction::CraftingTableInventoryInteraction(InventoryPlayer* pinv, WorldManager& worldMng,
                                                                     Runtime& gameRuntime, Int3 craftingTablePos)
    : CraftingInventoryInteraction(&m_sharedInventory, &m_craftInventory, pinv, gameRuntime, { 3, 3 }),
      m_world(worldMng), m_blockPosition(craftingTablePos) {
	m_sharedInventory.owner = this;
	mergeInventories();
}

CraftingTableInventoryInteraction::~CraftingTableInventoryInteraction() {
	writeBack();

	for (size_t i = 1; i <= m_gridSize.total(); i++) {
		ItemStack& stack = m_craftInventory.slots[i];
		if (stack.id == Items::Id::INVALID)
			continue;
		m_playerInventory->mergeItemStackInInventory(stack, true, 9, 44);
	}
}

void CraftingTableInventoryInteraction::writeBack() {
	size_t slotCount = 0;
	for (size_t i = 0; i < 10; i++)
		m_craftInventory.slots[i] = m_sharedInventory.slots[slotCount++];
	for (size_t i = 9; i < 45; i++)
		m_playerInventory->slots[i] = m_sharedInventory.slots[slotCount++];
}

bool CraftingTableInventoryInteraction::canExist() {
	return m_world.getBlockId(m_blockPosition) == BLOCK_CRAFTING_TABLE;
}

void CraftingTableInventoryInteraction::initSnapshot() {
	snapshot.resize(size_t(m_sharedInventory.getSizeInventory()));
	for (size_t i = 0; i < snapshot.size(); i++)
		snapshot[i] = m_sharedInventory.slots[i];
}

std::vector<DeltaSlot> CraftingTableInventoryInteraction::tickDiff() {
	std::vector<DeltaSlot> differences;
	mergeInventories(); // make sure sharedInventory is current before diffing
	for (size_t i = 0; i < snapshot.size(); i++) {
		auto& snap = snapshot[i];
		if (snap == m_sharedInventory.slots[i])
			continue;
		snap = m_sharedInventory.slots[i];
		differences.push_back({ snap, int(i) });
	}
	return differences;
}

void CraftingTableInventoryInteraction::mergeInventories() {
	size_t slotCount = 0;
	for (size_t i = 0; i < 10; i++)
		m_sharedInventory.slots[slotCount++] = m_craftInventory.slots[i];
	for (size_t i = 9; i < 45; i++)
		m_sharedInventory.slots[slotCount++] = m_playerInventory->slots[i];
}

void CraftingTableInventoryInteraction::updateResult() {
	CraftingInventoryInteraction::updateResult();
	mergeInventories();
}

void CraftingTableInventoryInteraction::shiftClickResult() {
	ItemStack& result = m_craftInventory.slots[0];
	if (result.id == Items::Id::INVALID)
		return;

	ItemStack copy = result;
	if (m_playerInventory->mergeItemStackInInventory(copy, true, 9, 44)) {
		finishCraft();
	} else {
		m_craftInventory.slots[0] = copy.count == 0 ? ItemStack{} : copy;
	}
}

void CraftingTableInventoryInteraction::shiftClickOther(int slot) {
	auto stack = m_sharedInventory.getStackInSlot(slot);
	if (!stack)
		return;

	ItemStack copy = *stack;

	if (slot < 10) {
		// Grid -> inventory
		// Try the main inventory then the hotbar
		bool success = m_playerInventory->mergeItemStackInInventory(copy, false, 9, 35);
		if (!success)
			m_playerInventory->mergeItemStackInInventory(copy, false, 36, 44);
	} else {
		// We can't shift click into the crafting grid itself, so just try the other area of the inventory
		// We shift clicked in the inventory
		if (slot > 9 && slot < 37) {
			m_playerInventory->mergeItemStackInInventory(copy, false, 36, 44);
		} else {
			m_playerInventory->mergeItemStackInInventory(copy, false, 9, 35);
		}
	}

	// Update the source in the real inventory before re-merging
	if (slot < 10) {
		m_craftInventory.slots[slot] = copy.count == 0 ? ItemStack{} : copy;
	} else {
		m_playerInventory->slots[slot - 1] = copy.count == 0 ? ItemStack{} : copy;
	}

	// Re-sync sharedInventory from the real inventories
	mergeInventories();
}