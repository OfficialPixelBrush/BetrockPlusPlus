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

struct Container {
    int windowId = 0;
    virtual void onCraftMatrixChanged(InventoryCrafting*) {}
    virtual bool canInteractWith(EntityPlayer* /*player*/) = 0;
    virtual ~Container() = default;
};

// InventoryPlayer
// 36 main + 4 armor = 40 slots. Hotbar is slots 0-8, armor is 36-39.
//
// Container slot layout (windowId=0, 45 slots total):
//   0        = craft output
//   1-4      = 2x2 craft grid
//   5-8      = armor (head, chest, legs, feet) -> inv slots 36-39
//   9-35     = main inventory rows 1-3          -> inv slots 9-35
//   36-44    = hotbar                           -> inv slots 0-8

struct InventoryPlayer : Inventory {
    int  currentItem = 0;
    bool inventoryChanged = false;
    EntityPlayer* player = nullptr;

    InventoryPlayer() : Inventory(40) { name = "Inventory"; }

    ItemStack* getCurrentItem() {
        if (currentItem < 0 || currentItem >= 9) return nullptr;
        return getStackInSlot(currentItem);
    }

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
            int take  = (std::min)(space, (int)stack.count);
            if (take <= 0) break;
            slot->count  = (int8_t)(slot->count + take);
            stack.count  = (int8_t)(stack.count - take);
            pickedUpAny  = true;
        }

        if (pickedUpAny) onInventoryChanged();
        return pickedUpAny;
    }

    bool consumeItem(int16_t id) {
        for (int i = 0; i < 36; i++) {
            if (slots[i].has_value() && slots[i]->id == id) {
                slots[i]->count = (int8_t)(slots[i]->count - 1);
                if (slots[i]->count <= 0) slots[i] = std::nullopt;
                onInventoryChanged();
                return true;
            }
        }
        return false;
    }

    void onInventoryChanged() override { inventoryChanged = true; }
    bool canInteractWith(EntityPlayer* /*player*/) override { return true; }

    // Shift-click within the player inventory (no external inventory open).
    ClickResult shiftClick(int16_t containerSlot) override {
        int invSlot = slotToInvSlot(containerSlot);
        if (invSlot < 0) return { ClickResultType::Rejected };

        ItemStack* src = getStackInSlot(invSlot);
        if (!src) return { ClickResultType::Noop };

        ItemStack working = *src;

        if (invSlot >= 9 && invSlot <= 35) {
            // Main -> hotbar
            shiftTransferInto(working, 0, 9, false);
        } else if (invSlot >= 0 && invSlot <= 8) {
            // Hotbar -> main
            shiftTransferInto(working, 9, 36, false);
        } else {
            // Armor -> main, then hotbar
            shiftTransferInto(working, 9, 36, false);
            if (working.count > 0) shiftTransferInto(working, 0, 9, false);
        }

        if (working.count <= 0) setInventorySlotContents(invSlot, nullptr);
        else src->count = working.count;

        return { ClickResultType::Accepted, {}, true }; // full resync
    }

protected:
    int slotToInvSlot(int16_t containerSlot) const override {
        if (containerSlot >= 0  && containerSlot <= 4)  return -1;              // craft output + grid
        if (containerSlot >= 5  && containerSlot <= 8)  return 36 + (containerSlot - 5); // armor
        if (containerSlot >= 9  && containerSlot <= 35) return containerSlot;             // main
        if (containerSlot >= 36 && containerSlot <= 44) return containerSlot - 36;        // hotbar
        return -1;
    }

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

// InventoryChes
// 27 slots, container slots 0-26 map directly to inventory slots 0-26.
// Cross-inventory shift-clicks (chest<->player) are handled by the packet
// handler since they span two Inventory instances.
struct InventoryChest : Inventory {
    InventoryChest() : Inventory(27) { name = "Chest"; }
};

// InventoryLargeChest
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

// InventoryDispenser
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

// InventoryFurnace
// Slots: [0] input, [1] fuel, [2] output.
struct InventoryFurnace : Inventory {
    int burnTime    = 0;
    int maxBurnTime = 0;
    int cookTime    = 0;

    InventoryFurnace() : Inventory(3) { name = "Furnace"; }
    bool isBurning() const { return burnTime > 0; }
};

// InventoryCrafting
struct InventoryCrafting : Inventory {
    int        width   = 0;
    int        height  = 0;
    Container* handler = nullptr;

    InventoryCrafting(Container* handler, int width, int height)
        : Inventory(width * height), width(width), height(height), handler(handler) {
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

// InventoryCraftResult
struct InventoryCraftResult : Inventory {
    InventoryCraftResult() : Inventory(1) { name = "Result"; }

    ItemStack decreaseStackSize(int slot, int /*count*/) override {
        if (slot != 0 || !slots[0].has_value()) return ItemStack{};
        ItemStack taken = slots[0].value();
        slots[0] = std::nullopt;
        return taken;
    }
};
