/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "large_chest.h"

LargeChestInventoryInteraction::LargeChestInventoryInteraction(InventoryPlayer* _pinv,
                                                               std::shared_ptr<TileEntityChest> _upper,
                                                               std::shared_ptr<TileEntityChest> _lower)
    : InventoryInteraction(&sharedInventory), playerInventory(_pinv), upperChest(_upper), lowerChest(_lower),
      chestInventory(&_upper->inventory, &_lower->inventory) {
	sharedInventory.owner = this;
	mergeInventories();
}

LargeChestInventoryInteraction::~LargeChestInventoryInteraction() {
	if (canExist())
		writeBack();
}

bool LargeChestInventoryInteraction::canExist() {
	return !upperChest.expired() && !lowerChest.expired();
}

void LargeChestInventoryInteraction::initSnapshot() {
	snapshot.resize(size_t(chestInventory.getSizeInventory()));
	for (size_t i = 0; i < size_t(chestInventory.getSizeInventory()); i++) {
		auto* stack = chestInventory.getStackInSlot(i);
		snapshot[i] = stack ? *stack : ItemStack{};
	}
}

// Analyze the snapshot vs the current chest inventory
std::vector<DeltaSlot> LargeChestInventoryInteraction::tickDiff() {
	std::vector<DeltaSlot> differences;
	for (size_t i = 0; i < snapshot.size(); i++) {
		auto* currentPtr = chestInventory.getStackInSlot(int(i));
		auto current = currentPtr ? *currentPtr : ItemStack{};

		if (snapshot[i] == current)
			continue;

		snapshot[i] = current;
		differences.push_back({ snapshot[i], int(i) });
	}
	mergeInventories();
	return differences;
}

void LargeChestInventoryInteraction::mergeInventories() {
	size_t slotCount = 0;
	for (size_t i = 0; i < size_t(chestInventory.getSizeInventory()); i++) {
		auto* stack = chestInventory.getStackInSlot(i);
		sharedInventory.slots[slotCount++] = stack ? *stack : ItemStack{};
	}
	for (size_t i = 9; i < 45; i++)
		sharedInventory.slots[slotCount++] = playerInventory->slots[i];
}

void LargeChestInventoryInteraction::writeBack() {
	for (size_t i = 0; i < 54; i++) {
		auto& slot = sharedInventory.slots[i];
		ItemStack* ptr = slot.id != Items::Id::INVALID ? &slot : nullptr;
		chestInventory.setInventorySlotContents(i, ptr);
	}
	for (size_t i = 54; i < 90; i++)
		playerInventory->slots[i - 54 + 9] = sharedInventory.slots[i];
}

void LargeChestInventoryInteraction::onShiftClick(int _slot) {
	auto stack = sharedInventory.getStackInSlot(_slot);
	if (!stack)
		return;

	ItemStack copy = *stack;

	if (_slot <= 53) {
		// Chest -> inventory
		[[maybe_unused]] bool success = playerInventory->mergeItemStackInInventory(copy, true, 9, 44);
	} else {
		// Inventory -> Chest
		[[maybe_unused]] bool success = chestInventory.mergeItemStackInInventory(copy);
	}

	// Update the source in the real inventory before re-merging
	if (_slot <= 53) {
		ItemStack* ptr = copy.count == 0 ? nullptr : &copy;
		chestInventory.setInventorySlotContents(_slot, ptr);
	} else {
		int playerSlot = _slot - 54 + 9;
		playerInventory->slots[size_t(playerSlot)] = copy.count == 0 ? ItemStack{} : copy;
	}

	// Re-sync sharedInventory from the real inventories
	mergeInventories();
}
