/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "inventory_interaction.h"

InventoryInteraction::InventoryInteraction(Inventory* inv) : inventory(inv) {}

bool InventoryInteraction::canExist() {
	return inventory != nullptr;
}

void InventoryInteraction::initSnapshot() {
	snapshot = inventory->slots;
}

std::vector<DeltaSlot> InventoryInteraction::tickDiff() {
	std::vector<DeltaSlot> differences;
	for (size_t i = 0; i < snapshot.size(); i++) {
		[[maybe_unused]] auto* current = inventory->getStackInSlot(i);
		auto& snap = snapshot[i];

		bool changed = snap != inventory->slots[i];
		if (!changed)
			continue;

		snap = inventory->slots[i];
		differences.push_back({ snap, int(i) });
	}
	return differences;
}

void InventoryInteraction::onLeftClick(int slot) {
	auto targetSlot = inventory->getStackInSlot(slot);

	// Empty slot
	if (!targetSlot) {
		if (carried.id != ITEM_INVALID) {
			inventory->setInventorySlotContents(slot, &carried);
			carried = ItemStack{};
		}
		inventory->onInventoryChanged();
		return;
	}

	// Not carrying anything
	if (carried.id == ITEM_INVALID) {
		carried = *targetSlot;
		inventory->clearSlot(slot);
		inventory->onInventoryChanged();
		return;
	}

	// Same item; merge
	if (targetSlot->id == carried.id && targetSlot->data == carried.data) {
		int maxStack = GetMaxStack(targetSlot->id);
		int space = maxStack - targetSlot->count;
		int toMove = std::min(space, (int)carried.count);
		targetSlot->count += toMove;
		carried.count -= toMove;
		if (carried.count == 0)
			carried = ItemStack{};
		inventory->onInventoryChanged();
		return;
	}

	// Different item; swap
	ItemStack temp = *targetSlot;
	*targetSlot = carried;
	carried = temp;
	inventory->onInventoryChanged();
}

void InventoryInteraction::onRightClick(int slot) {
	auto targetSlot = inventory->getStackInSlot(slot);

	if (carried.id != ITEM_INVALID) {
		if (!targetSlot) {
			ItemStack single{ carried.id, 1, carried.data };
			inventory->setInventorySlotContents(slot, &single);
			carried.count -= 1;
			if (carried.count == 0)
				carried = ItemStack{};
			inventory->onInventoryChanged();
			return;
		}

		// If we right click on the same item we are carrying just add one
		if (targetSlot->id == carried.id && targetSlot->data == carried.data) {
			int maxStack = GetMaxStack(targetSlot->id);
			int space = maxStack - targetSlot->count;
			if (space >= 1) {
				targetSlot->count += 1;
				carried.count -= 1;
				if (carried.count == 0)
					carried = ItemStack{};
				inventory->onInventoryChanged();
			}
			return;
		}

		// If we right click on a different item, swap the cursor and that item
		ItemStack temp = *targetSlot;
		*targetSlot = carried;
		carried = temp;
		inventory->onInventoryChanged();
		return;
	}

	if (!targetSlot)
		return;

	// Only split items if there stack count is greater than 1 and we aren't carrying anything
	if (targetSlot->count > 1) {
		// Beta always take the higher of the two if uneven
		int taken = (targetSlot->count + 1) / 2;
		int left = targetSlot->count - taken;
		targetSlot->count = int8_t(left);
		carried = ItemStack{ targetSlot->id, int8_t(taken), targetSlot->data };
		inventory->onInventoryChanged();
		return;
	}

	// If its only one item we just pick it up
	carried = *targetSlot;
	inventory->clearSlot(slot);
	inventory->onInventoryChanged();
	return;
}

void InventoryInteraction::onShiftClick(int slot) {
	auto targetSlot = inventory->getStackInSlot(slot);
	if (!targetSlot)
		return;
	inventory->mergeItemStackInInventory(*targetSlot);
}
