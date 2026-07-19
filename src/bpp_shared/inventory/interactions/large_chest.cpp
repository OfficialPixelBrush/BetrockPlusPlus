/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "large_chest.h"

LargeChestInventoryInteraction::LargeChestInventoryInteraction(InventoryPlayer* pinv,
                                                               std::shared_ptr<TileEntityChest> upper,
                                                               std::shared_ptr<TileEntityChest> lower)
    : InventoryInteraction(&m_sharedInventory), m_playerInventory(pinv), m_upperChest(upper), m_lowerChest(lower),
      m_chestInventory(&upper->m_inventory, &lower->m_inventory) {
	m_sharedInventory.m_owner = this;
	mergeInventories();
}

LargeChestInventoryInteraction::~LargeChestInventoryInteraction() {
	if (canExist())
		writeBack();
}

bool LargeChestInventoryInteraction::canExist() {
	return !m_upperChest.expired() && !m_lowerChest.expired();
}

void LargeChestInventoryInteraction::initSnapshot() {
	m_snapshot.resize(size_t(m_chestInventory.getSizeInventory()));
	for (size_t i = 0; i < size_t(m_chestInventory.getSizeInventory()); i++) {
		auto* stack = m_chestInventory.getStackInSlot(i);
		m_snapshot[i] = stack ? *stack : ItemStack{};
	}
}

// Analyze the snapshot vs the current chest inventory
std::vector<DeltaSlot> LargeChestInventoryInteraction::tickDiff() {
	std::vector<DeltaSlot> differences;
	for (size_t i = 0; i < m_snapshot.size(); i++) {
		auto* currentPtr = m_chestInventory.getStackInSlot(int(i));
		auto current = currentPtr ? *currentPtr : ItemStack{};

		if (m_snapshot[i] == current)
			continue;

		m_snapshot[i] = current;
		differences.push_back({ m_snapshot[i], int(i) });
	}
	mergeInventories();
	return differences;
}

void LargeChestInventoryInteraction::mergeInventories() {
	size_t slotCount = 0;
	for (size_t i = 0; i < size_t(m_chestInventory.getSizeInventory()); i++) {
		auto* stack = m_chestInventory.getStackInSlot(i);
		m_sharedInventory.m_slots[slotCount++] = stack ? *stack : ItemStack{};
	}
	for (size_t i = 9; i < 45; i++)
		m_sharedInventory.m_slots[slotCount++] = m_playerInventory->m_slots[i];
}

void LargeChestInventoryInteraction::writeBack() {
	for (size_t i = 0; i < 54; i++) {
		auto& slot = m_sharedInventory.m_slots[i];
		ItemStack* ptr = slot.m_id != Items::Id::INVALID ? &slot : nullptr;
		m_chestInventory.setInventorySlotContents(i, ptr);
	}
	for (size_t i = 54; i < 90; i++)
		m_playerInventory->m_slots[i - 54 + 9] = m_sharedInventory.m_slots[i];
}

void LargeChestInventoryInteraction::onShiftClick(int slot) {
	auto stack = m_sharedInventory.getStackInSlot(slot);
	if (!stack)
		return;

	ItemStack copy = *stack;

	if (slot <= 53) {
		// Chest -> inventory
		[[maybe_unused]] bool success = m_playerInventory->mergeItemStackInInventory(copy, true, 9, 44);
	} else {
		// Inventory -> Chest
		[[maybe_unused]] bool success = m_chestInventory.mergeItemStackInInventory(copy);
	}

	// Update the source in the real inventory before re-merging
	if (slot <= 53) {
		ItemStack* ptr = copy.m_count == 0 ? nullptr : &copy;
		m_chestInventory.setInventorySlotContents(slot, ptr);
	} else {
		int playerSlot = slot - 54 + 9;
		m_playerInventory->m_slots[size_t(playerSlot)] = copy.m_count == 0 ? ItemStack{} : copy;
	}

	// Re-sync sharedInventory from the real inventories
	mergeInventories();
}
