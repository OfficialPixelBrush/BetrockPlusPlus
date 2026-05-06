/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once
#include "inventories.h"

// Used for actually interacting with inventories, will typically wrap 1 or more inventory objects for things like chests, etc
struct InventoryInteraction {
	std::optional<ItemStack> carried;
	Inventory inventory;

    void onLeftClick(int slot) {
        auto targetSlot = inventory.getStackInSlot(slot);

        // Empty slot
        if (!targetSlot) {
            if (carried.has_value()) {
                inventory.setInventorySlotContents(slot, &carried.value());
                carried = std::nullopt;
            }
            return;
        }

        // Not carrying anything
        if (!carried.has_value()) {
            carried = *targetSlot;
            inventory.clearSlot(slot);
            return;
        }

        // Same item; merge
        if (targetSlot->id == carried->id && targetSlot->data == carried->data) {
            int maxStack = GetMaxStack(targetSlot->id);
            int space = maxStack - targetSlot->count;

            int toMove = std::min(space, (int)carried->count);

            targetSlot->count += toMove;
            carried->count -= toMove;

            if (carried->count == 0) {
                carried = std::nullopt;
            }

            inventory.onInventoryChanged();
            return;
        }

        // Different item; swap
        ItemStack temp = *targetSlot;
        *targetSlot = *carried;
        carried = temp;

        inventory.onInventoryChanged();
    }

	virtual void onRightClick(int slot) {

	}

	virtual void onShiftClick(int slot) {

	}
};