/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "chest.h"

ChestInventoryInteraction::ChestInventoryInteraction(InventoryPlayer* pinv, std::shared_ptr<TileEntityChest> chest)
    : InventoryInteraction(&m_sharedInventory), m_playerInventory(pinv), m_chestHandle(chest),
      m_chestInventory(&chest->m_inventory) {
	m_sharedInventory.m_owner = this;
	mergeInventories();
}

ChestInventoryInteraction::~ChestInventoryInteraction() {
	if (canExist())
		writeBack();
}

bool ChestInventoryInteraction::canExist() {
	return !m_chestHandle.expired();
}

void ChestInventoryInteraction::initSnapshot() {
	m_snapshot.resize(size_t(m_chestInventory->getSizeInventory()));
	for (size_t i = 0; i < size_t(m_chestInventory->getSizeInventory()); i++)
		m_snapshot[i] = m_chestInventory->m_slots[i];
}

// Analyze the snapshot vs the current chest inventory
std::vector<DeltaSlot> ChestInventoryInteraction::tickDiff() {
	std::vector<DeltaSlot> differences;
	for (size_t i = 0; i < m_snapshot.size(); i++) {
		[[maybe_unused]] auto* current = m_chestInventory->getStackInSlot(i);
		auto& snap = m_snapshot[i];

		bool changed = snap != m_chestInventory->m_slots[i];
		if (!changed)
			continue;

		snap = m_chestInventory->m_slots[i];
		differences.push_back({ snap, int(i) });
	}
	mergeInventories();
	return differences;
}

void ChestInventoryInteraction::mergeInventories() {
	size_t slotCount = 0;
	for (auto& slot : m_chestInventory->m_slots)
		m_sharedInventory.m_slots[slotCount++] = slot;
	for (size_t i = 9; i < 45; i++)
		m_sharedInventory.m_slots[slotCount++] = m_playerInventory->m_slots[i];
}

void ChestInventoryInteraction::writeBack() {
	for (size_t i = 0; i < 27; i++)
		m_chestInventory->m_slots[i] = m_sharedInventory.m_slots[i];
	for (size_t i = 27; i < 63; i++)
		m_playerInventory->m_slots[i - 27 + 9] = m_sharedInventory.m_slots[i];
}

void ChestInventoryInteraction::onShiftClick(int slot) {
	auto stack = m_sharedInventory.getStackInSlot(slot);
	if (!stack)
		return;

	ItemStack copy = *stack;

	if (slot <= 26) {
		// Chest -> inventory
		[[maybe_unused]] bool success = m_playerInventory->mergeItemStackInInventory(copy, true, 9, 44);
	} else {
		// Inventory -> Chest
		[[maybe_unused]] bool success = m_chestInventory->mergeItemStackInInventory(copy);
	}

	// Update the source in the real inventory before re-merging
	if (slot <= 26) {
		m_chestInventory->m_slots[size_t(slot)] = copy.m_count == 0 ? ItemStack{} : copy;
	} else {
		int playerSlot = slot - 27 + 9;
		m_playerInventory->m_slots[size_t(playerSlot)] = copy.m_count == 0 ? ItemStack{} : copy;
	}

	// Re-sync sharedInventory from the real inventories
	mergeInventories();
}
