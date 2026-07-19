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
    : CraftingInventoryInteraction(inv, inv, inv, gameRuntime, { 2, 2 }), m_playerInventory(inv) {}

bool PlayerInventoryInteraction::canExist() {
	return m_playerInventory != nullptr;
}

void PlayerInventoryInteraction::shiftClickResult() {
	ItemStack& result = m_playerInventory->m_slots[0];
	if (result.m_id == Items::Id::INVALID)
		return;

	ItemStack copy = result;
	if (m_playerInventory->mergeItemStackInInventory(copy, true, 9, 44)) {
		finishCraft();
	} else {
		m_playerInventory->m_slots[0] = copy.m_count == 0 ? ItemStack{} : copy;
	}
}

void PlayerInventoryInteraction::shiftClickOther(int slot) {
	auto from = m_playerInventory->getInventoryAreaFromSlot(slot);
	auto stack = m_playerInventory->getStackInSlot(slot);
	if (!stack)
		return;

	ItemStack copy = *stack; // work on a copy so we can detect what moved

	if (from == InvMap::ARMOR || from == InvMap::CRAFTING_AREA || from == InvMap::CRAFTING_RESULT ||
	    from == InvMap::HOTBAR) {
		bool success = m_playerInventory->mergeItemStackInInventory(copy, false, 9, 35);
		if (!success)
			m_playerInventory->mergeItemStackInInventory(copy, false, 36, 44);
	} else {
		m_playerInventory->mergeItemStackInInventory(copy, false, 36, 44);
	}

	// Update the source slot to whatever count is left
	if (copy.m_count == 0) {
		m_playerInventory->clearSlot(slot);
	} else {
		stack->m_count = copy.m_count;
	}
}