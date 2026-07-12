/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "player.h"
#include "../../entities/entity_player.h"
#include "inventory/interactions/crafting.h"
#include "inventory/inventories.h"

PlayerInventoryInteraction::PlayerInventoryInteraction(InventoryPlayer* inv, Runtime& gameRuntime)
    : CraftingInventoryInteraction(inv, inv, inv, gameRuntime, {2, 2}), playerInventory(inv) {}

bool PlayerInventoryInteraction::canExist() {
	return playerInventory != nullptr;
}

void PlayerInventoryInteraction::shiftClickResult() {
	ItemStack& result = playerInventory->slots[0];
	if (result.id == ITEM_INVALID)
		return;

	ItemStack copy = result;
	if (playerInventory->mergeItemStackInInventory(copy, true, 9, 44)) {
		finishCraft();
	} else {
		playerInventory->slots[0] = copy.count == 0 ? ItemStack{} : copy;
	}
}

void PlayerInventoryInteraction::shiftClickOther(int slot) {
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