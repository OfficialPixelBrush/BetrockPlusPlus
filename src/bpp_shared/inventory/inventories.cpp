/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#include "inventories.h"

void InventoryCrafting::setInventorySlotContents(int slot, ItemStack* stack) {
    if (slot < 0 || slot >= (int)slots.size()) return;
    slots[slot] = stack ? std::optional<ItemStack>(*stack) : std::nullopt;
    if (handler) handler->onCraftMatrixChanged(this);
}

ItemStack InventoryCrafting::decreaseStackSize(int slot, int count) {
    ItemStack taken = Inventory::decreaseStackSize(slot, count);
    if (handler) handler->onCraftMatrixChanged(this);
    return taken;
}