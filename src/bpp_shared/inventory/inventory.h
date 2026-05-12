/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once
#include "item_stack.h"
#include "enums/items.h"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <algorithm>

struct ItemStack;
struct EntityPlayer;

// Inventory
struct Inventory {
    std::string name = "Inventory";
    std::vector<std::optional<ItemStack>> slots;

    Inventory(size_t size) : slots(size) {}

    virtual int getSizeInventory() const {
        return int(slots.size());
    }

    virtual int getNetworkSlotId(int slot) const {
        if (slot < 0 || slot >= int(slots.size())) return -1;
        return slot;
    }

    virtual ItemStack* getStackInSlot(int slot) {
        if (slot < 0 || slot >= (int)slots.size()) return nullptr;
        if (!slots[size_t(slot)].has_value()) return nullptr;
        return &slots[size_t(slot)].value();
    }

    virtual ItemStack decreaseStackSize(int slot, int count) {
        if (slot < 0 || slot >= (int)slots.size() || !slots[size_t(slot)].has_value())
            return ItemStack{};
        auto& stack = slots[size_t(slot)].value();
        if (stack.count <= count) {
            ItemStack taken = stack;
            slots[size_t(slot)] = std::nullopt;
            onInventoryChanged();
            return taken;
        }
        ItemStack taken{ stack.id, (int8_t)count, stack.data };
        stack.count = (int8_t)(stack.count - count);
        onInventoryChanged();
        return taken;
    }

    virtual void setInventorySlotContents(int slot, ItemStack* stack) {
        if (slot < 0 || slot >= (int)slots.size()) return;
        slots[size_t(slot)] = stack ? std::optional<ItemStack>(*stack) : std::nullopt;
        onInventoryChanged();
    }

    virtual const std::string& getInventoryName() const { return name; }
    virtual void onInventoryChanged() {}
    virtual ~Inventory() = default;

    void clearSlot(int slot) {
        if (slot < 0 || slot >= (int)slots.size()) return;
        slots[size_t(slot)] = std::nullopt;
        onInventoryChanged();
    }

    // Take in the original item stack, try and merge it with our inventory. Returns if it was successful
    // Start slot and end slot are inclusive
    virtual bool mergeItemStackInInventory(ItemStack& stack, bool reverse = false, int startSlot = 0, int endSlot = -1) {
        auto start = startSlot;
        auto end = endSlot == -1 ? getSizeInventory() - 1 : endSlot;

        // Try and merge into an already existing stack of the same type if this item is stackable
        if (IsStackable(stack.id)) {
            for (int i = reverse ? end : start; reverse ? i >= start : i <= end; reverse ? i-- : i++) {
                auto slot = getStackInSlot(i);
                if (!slot) continue;
                if (slot->id == stack.id && slot->data == stack.data) {
                    auto maxStack = GetMaxStack(slot->id);
                    // Don't try and merge into an already maxed out stack
                    if (slot->count >= maxStack) continue;

                    // Add the stacks together and do some checks to make sure we don't overflow
                    int space = maxStack - slot->count;
                    int toMove = std::min(space, (int)stack.count);

                    slot->count += toMove;
                    stack.count -= toMove;

                    onInventoryChanged();
                    if (stack.count == 0) return true;
                }
            }
        }

        // We couldn't merge into existing items so just try and find an empty slot
        for (int i = reverse ? end : start; reverse ? i >= start : i <= end; reverse ? i-- : i++) {
            if (!slots[size_t(i)].has_value()) {
                slots[size_t(i)] = ItemStack{ stack.id, stack.count, stack.data };
                stack.id = ITEM_INVALID;
                stack.data = 0;
                stack.count = 0;
                onInventoryChanged();
                return true;
            }
        }
        return false; // Give up
    }
};
