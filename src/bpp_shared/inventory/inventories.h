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

struct InventoryCrafting; // forward decl

struct Container {
    int windowId = 0;
    virtual void onCraftMatrixChanged(InventoryCrafting*) {}
    virtual bool canInteractWith(EntityPlayer* /*player*/) = 0;
    virtual ~Container() = default;
};

// Player inventory -- 36 main + 4 armor = 40 slots
// Armor slots are at indices 36-39. Hotbar is slots 0-8.
struct InventoryPlayer : Inventory {
    int  currentItem = 0;     // hotbar index (0-8)
    bool inventoryChanged = false;
    std::optional<ItemStack> carried; // cursor stack during GUI interaction
    EntityPlayer* player = nullptr;

    InventoryPlayer() : Inventory(40) { name = "Inventory"; }

    // Returns the currently held item (hotbar slot currentItem), or nullptr.
    ItemStack* getCurrentItem() {
        if (currentItem < 0 || currentItem >= 9) return nullptr;
        return getStackInSlot(currentItem);
    }

    // Tries to add a stack to the inventory.
    // Non-stackable/damageable items go to the first empty slot.
    // Stackable items merge into existing partial stacks first, then empty slots.
    // Returns true if at least one item was picked up.
    bool addItemStackToInventory(ItemStack stack) {
        if (GetMaxDurability(stack.id) > 0 || GetMaxStack(stack.id) == 1) {
            for (int i = 0; i < 36; i++) {
                if (!slots[i].has_value()) {
                    slots[i] = stack;
                    onInventoryChanged();
                    return true;
                }
            }
            return false;
        }

        bool pickedUpAny = false;
        while (stack.count > 0) {
            int target = findPartialStack(stack);
            if (target < 0) target = findEmptySlot();
            if (target < 0) break;

            auto& slot = slots[target];
            if (!slot.has_value())
                slot = ItemStack{ stack.id, 0, stack.data };

            int space = GetMaxStack(stack.id) - (int)slot->count;
            int take = (std::min)(space, (int)stack.count);
            if (take <= 0) break;
            slot->count = (int8_t)(slot->count + take);
            stack.count = (int8_t)(stack.count - take);
            pickedUpAny = true;
        }

        if (pickedUpAny) onInventoryChanged();
        return pickedUpAny;
    }

    // Consume one item of the given id from anywhere in the main inventory.
    bool consumeItem(int16_t id) {
        for (int i = 0; i < 36; i++) {
            if (slots[i].has_value() && slots[i]->id == id) {
                slots[i]->count = (int8_t)(slots[i]->count - 1);
                if (slots[i]->count <= 0)
                    slots[i] = std::nullopt;
                onInventoryChanged();
                return true;
            }
        }
        return false;
    }

    void onInventoryChanged() override { inventoryChanged = true; }
    bool canInteractWith(EntityPlayer* /*player*/) override { return true; }

private:
    int findPartialStack(const ItemStack& stack) {
        for (int i = 0; i < 36; i++) {
            if (!slots[i].has_value()) continue;
            auto& s = slots[i].value();
            if (s.id == stack.id && s.data == stack.data && s.count < GetMaxStack(s.id))
                return i;
        }
        return -1;
    }

    int findEmptySlot() {
        for (int i = 0; i < 36; i++)
            if (!slots[i].has_value()) return i;
        return -1;
    }
};

// Chest -- 27 slots
struct InventoryChest : Inventory {
    InventoryChest() : Inventory(27) { name = "Chest"; }
};

// Large chest -- two chests merged into one 54-slot view.
// Does not own slots; delegates to two Inventory instances.
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

// Dispenser -- 9 slots.
// getRandomStack() removes and returns one item from a random non-empty slot.
struct InventoryDispenser : Inventory {
    std::mt19937 rng{ std::random_device{}() };

    InventoryDispenser() : Inventory(9) { name = "Trap"; }

    std::optional<ItemStack> getRandomStack() {
        int chosen = -1;
        int weight = 1;
        for (int i = 0; i < 9; i++) {
            if (!slots[i].has_value()) continue;
            if (std::uniform_int_distribution<int>(0, weight++ - 1)(rng) == 0)
                chosen = i;
        }
        if (chosen < 0) return std::nullopt;
        return decreaseStackSize(chosen, 1);
    }
};

// Furnace -- 3 slots: [0] input, [1] fuel, [2] output.
// Ticking logic lives in TileEntityFurnace; this is just the data.
struct InventoryFurnace : Inventory {
    int burnTime = 0; // ticks of fuel remaining
    int maxBurnTime = 0; // total burn time of current fuel item
    int cookTime = 0; // ticks cooking so far (max 200)

    InventoryFurnace() : Inventory(3) { name = "Furnace"; }

    bool isBurning() const { return burnTime > 0; }
};

// Crafting grid -- NxM slots, notifies a Container on any change.
// Container pointer is non-owning; container outlives the grid.
struct InventoryCrafting : Inventory {
    int        width = 0;
    int        height = 0;
    Container* handler = nullptr;

    InventoryCrafting(Container* handler, int width, int height)
        : Inventory(width* height), width(width), height(height), handler(handler) {
        name = "Crafting";
    }

    ItemStack* getStackAt(int x, int y) {
        if (x < 0 || x >= width || y < 0 || y >= height) return nullptr;
        return getStackInSlot(x + y * width);
    }

    void setInventorySlotContents(int slot, ItemStack* stack) override;
    ItemStack decreaseStackSize(int slot, int count) override;
    void onInventoryChanged() override {}
};

// Craft result -- single output slot, always taken whole.
struct InventoryCraftResult : Inventory {
    InventoryCraftResult() : Inventory(1) { name = "Result"; }

    ItemStack decreaseStackSize(int slot, int /*count*/) override {
        if (slot != 0 || !slots[0].has_value()) return ItemStack{};
        ItemStack taken = slots[0].value();
        slots[0] = std::nullopt;
        return taken;
    }
};