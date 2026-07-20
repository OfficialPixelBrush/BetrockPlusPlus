/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "chest.h"

ChestInventoryInteraction::ChestInventoryInteraction(InventoryPlayer* pinv, std::shared_ptr<TileEntityChest> chest)
    : InventoryInteraction(&sharedInventory), playerInventory(pinv), chestHandle(chest),
      chestInventory(&chest->inventory) {
	sharedInventory.owner = this;
	mergeInventories();
}

ChestInventoryInteraction::~ChestInventoryInteraction() {
	if (canExist())
		writeBack();
}

bool ChestInventoryInteraction::canExist() {
	return !chestHandle.expired();
}

void ChestInventoryInteraction::initSnapshot() {
	snapshot.resize(size_t(chestInventory->getSizeInventory()));
	for (size_t i = 0; i < size_t(chestInventory->getSizeInventory()); i++)
		snapshot[i] = chestInventory->slots[i];
}

// Analyze the snapshot vs the current chest inventory
std::vector<DeltaSlot> ChestInventoryInteraction::tickDiff() {
	std::vector<DeltaSlot> differences;
	for (size_t i = 0; i < snapshot.size(); i++) {
		[[maybe_unused]] auto* current = chestInventory->getStackInSlot(i);
		auto& snap = snapshot[i];

		bool changed = snap != chestInventory->slots[i];
		if (!changed)
			continue;

		snap = chestInventory->slots[i];
		differences.push_back({ snap, int(i) });
	}
	mergeInventories();
	return differences;
}

void ChestInventoryInteraction::mergeInventories() {
	size_t slotCount = 0;
	for (auto& slot : chestInventory->slots)
		sharedInventory.slots[slotCount++] = slot;
	for (size_t i = 9; i < 45; i++)
		sharedInventory.slots[slotCount++] = playerInventory->slots[i];
}

void ChestInventoryInteraction::writeBack() {
	for (size_t i = 0; i < 27; i++)
		chestInventory->slots[i] = sharedInventory.slots[i];
	for (size_t i = 27; i < 63; i++)
		playerInventory->slots[i - 27 + 9] = sharedInventory.slots[i];
}

void ChestInventoryInteraction::onShiftClick(int slot) {
	auto stack = sharedInventory.getStackInSlot(slot);
	if (!stack)
		return;

	ItemStack copy = *stack;

	if (slot <= 26) {
		// Chest -> inventory
		[[maybe_unused]] bool success = playerInventory->mergeItemStackInInventory(copy, true, 9, 44);
	} else {
		// Inventory -> Chest
		[[maybe_unused]] bool success = chestInventory->mergeItemStackInInventory(copy);
	}

	// Update the source in the real inventory before re-merging
	if (slot <= 26) {
		chestInventory->slots[size_t(slot)] = copy.count == 0 ? ItemStack{} : copy;
	} else {
		int playerSlot = slot - 27 + 9;
		playerInventory->slots[size_t(playerSlot)] = copy.count == 0 ? ItemStack{} : copy;
	}

	// Re-sync sharedInventory from the real inventories
	mergeInventories();
}
