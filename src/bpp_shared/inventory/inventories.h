/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#pragma once
#include "inventory.h"
#include "item_stack.h"
#include "enums/items.h"
#include <algorithm>
#include <optional>
#include <random>

struct EntityPlayer;
struct Container;
struct InventoryCrafting;

enum invMap {
    armor,
    inventory,
    hotbar,
    crafting,
    craftingResult,
    outOfBounds
};

// Network format (The rest of the inventories are self explanatory this is the only one that is semi-convoluted):
// Slots 5 -> 8 are for armor
// Slots 36 -> 44 are the hotbar
// Slots 9 -> 35 are the main inventory
// Slots 1 -> 4 are the crafting grid
// Slot 0 is the crafting result
struct InventoryPlayer : Inventory {
public:
    int  currentItem = 0;
    bool inventoryChanged = false;
    EntityPlayer* player = nullptr;

    InventoryPlayer() : Inventory(45) { name = "Inventory"; }

    ItemStack* getCurrentItem() {
        if (currentItem < 0 || currentItem >= 9) return nullptr;
        return getStackInSlot(currentItem);
    }

    invMap getInventoryAreaFromSlot(int slot) {
        if (slot == 0) return invMap::craftingResult;
        if (slot >= 1 && slot <= 4) return invMap::crafting;
        if (slot >= 5 && slot <= 8) return invMap::armor;
        if (slot >= 36 && slot <= 44) return invMap::hotbar;
        if (slot >= 9 && slot <= 35) return invMap::inventory;
    }

    void onInventoryChanged() override { inventoryChanged = true; }

private:
};

struct InventoryChest : Inventory {
    InventoryChest() : Inventory(27) { name = "Chest"; }
};

// Just a wrapper for two chest inventories
struct InventoryLargeChest : Inventory {
    Inventory* upper = nullptr;
    Inventory* lower = nullptr;

    InventoryLargeChest(Inventory* upper, Inventory* lower)
        : Inventory(0), upper(upper), lower(lower) {
        name = "Large Chest";
    }

    int getSizeInventory() const override {
        return upper->getSizeInventory() + lower->getSizeInventory();
    }

    ItemStack* getStackInSlot(int slot) override {
        int upperSize = upper->getSizeInventory();
        if (slot < upperSize) return upper->getStackInSlot(slot);
        return lower->getStackInSlot(slot - upperSize);
    }

    ItemStack decreaseStackSize(int slot, int count) override {
        int upperSize = upper->getSizeInventory();
        if (slot < upperSize) return upper->decreaseStackSize(slot, count);
        return lower->decreaseStackSize(slot - upperSize, count);
    }

    void setInventorySlotContents(int slot, ItemStack* stack) override {
        int upperSize = upper->getSizeInventory();
        if (slot < upperSize) upper->setInventorySlotContents(slot, stack);
        else lower->setInventorySlotContents(slot - upperSize, stack);
    }

    void onInventoryChanged() override {
        upper->onInventoryChanged();
        lower->onInventoryChanged();
    }
};

struct InventoryDispenser : Inventory {
    std::mt19937 rng{ std::random_device{}() };

    InventoryDispenser() : Inventory(9) { name = "Trap"; }

    std::optional<ItemStack> getRandomStack() {
        int chosen = -1, weight = 1;
        for (int i = 0; i < 9; i++) {
            if (!slots[i].has_value()) continue;
            if (std::uniform_int_distribution<int>(0, weight++ - 1)(rng) == 0)
                chosen = i;
        }
        if (chosen < 0) return std::nullopt;
        return decreaseStackSize(chosen, 1);
    }
};

// Slots: 0 = input, 1 = fuel, 2 = output.
struct InventoryFurnace : Inventory {
    int burnTime    = 0;
    int maxBurnTime = 0;
    int cookTime    = 0;

    InventoryFurnace() : Inventory(3) { name = "Furnace"; }
    bool isBurning() const { return burnTime > 0; }
};