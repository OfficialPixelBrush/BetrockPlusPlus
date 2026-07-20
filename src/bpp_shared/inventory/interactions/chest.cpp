/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "chest.h"

ChestInventoryInteraction::ChestInventoryInteraction(InventoryPlayer* _pinv, std::shared_ptr<TileEntityChest> _chest)
    : InventoryInteraction(&sharedInventory), playerInventory(_pinv), chestHandle(_chest),
      chestInventory(&_chest->inventory) {
	sharedInventory.owner = this;
	MergeInventories();
}

ChestInventoryInteraction::~ChestInventoryInteraction() {
	if (CanExist())
		WriteBack();
}

bool ChestInventoryInteraction::CanExist() {
	return !chestHandle.expired();
}

void ChestInventoryInteraction::InitSnapshot() {
	snapshot.resize(size_t(chestInventory->GetSizeInventory()));
	for (size_t i = 0; i < size_t(chestInventory->GetSizeInventory()); i++)
		snapshot[i] = chestInventory->slots[i];
}

// Analyze the snapshot vs the current chest inventory
std::vector<DeltaSlot> ChestInventoryInteraction::TickDiff() {
	std::vector<DeltaSlot> differences;
	for (size_t i = 0; i < snapshot.size(); i++) {
		[[maybe_unused]] auto* current = chestInventory->GetStackInSlot(i);
		auto& snap = snapshot[i];

		bool changed = snap != chestInventory->slots[i];
		if (!changed)
			continue;

		snap = chestInventory->slots[i];
		differences.push_back({ snap, int(i) });
	}
	MergeInventories();
	return differences;
}

void ChestInventoryInteraction::MergeInventories() {
	size_t slotCount = 0;
	for (auto& slot : chestInventory->slots)
		sharedInventory.slots[slotCount++] = slot;
	for (size_t i = 9; i < 45; i++)
		sharedInventory.slots[slotCount++] = playerInventory->slots[i];
}

void ChestInventoryInteraction::WriteBack() {
	for (size_t i = 0; i < 27; i++)
		chestInventory->slots[i] = sharedInventory.slots[i];
	for (size_t i = 27; i < 63; i++)
		playerInventory->slots[i - 27 + 9] = sharedInventory.slots[i];
}

void ChestInventoryInteraction::OnShiftClick(int _slot) {
	auto stack = sharedInventory.GetStackInSlot(_slot);
	if (!stack)
		return;

	ItemStack copy = *stack;

	if (_slot <= 26) {
		// Chest -> inventory
		[[maybe_unused]] bool success = playerInventory->MergeItemStackInInventory(copy, true, 9, 44);
	} else {
		// Inventory -> Chest
		[[maybe_unused]] bool success = chestInventory->MergeItemStackInInventory(copy);
	}

	// Update the source in the real inventory before re-merging
	if (_slot <= 26) {
		chestInventory->slots[size_t(_slot)] = copy.count == 0 ? ItemStack{} : copy;
	} else {
		int playerSlot = _slot - 27 + 9;
		playerInventory->slots[size_t(playerSlot)] = copy.count == 0 ? ItemStack{} : copy;
	}

	// Re-sync sharedInventory from the real inventories
	MergeInventories();
}
