/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "player.h"

PlayerInventoryInteraction::PlayerInventoryInteraction(InventoryPlayer* inv)
    : InventoryInteraction(inv), playerInventory(inv) {}

bool PlayerInventoryInteraction::canExist() {
	return playerInventory != nullptr;
}

void PlayerInventoryInteraction::onShiftClick(int slot) {
	auto from = playerInventory->getInventoryAreaFromSlot(slot);
	auto stack = playerInventory->getStackInSlot(slot);
	if (!stack)
		return;

	ItemStack copy = *stack; // work on a copy so we can detect what moved

	if (from == InvMap::ARMOR || from == InvMap::CRAFTING_AREA || from == InvMap::CRAFTING_RESULT ||
	    from == InvMap::HOTBAR) {
		bool success = playerInventory->mergeItemStackInInventory(copy, false, 9, 35);
		if (!success)
			playerInventory->mergeItemStackInInventory(copy, false, 36, 44);
	} else {
		playerInventory->mergeItemStackInInventory(copy, false, 36, 44);
	}

	// Update the source slot to whatever count is left
	if (copy.count == 0) {
		playerInventory->clearSlot(slot);
	} else {
		stack->count = copy.count;
	}
}