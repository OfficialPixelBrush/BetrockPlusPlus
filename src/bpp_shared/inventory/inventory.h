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
    std::optional<ItemStack> carried;

    Inventory(int size) : slots(size) {}

    virtual int getSizeInventory() const {
        return (int)slots.size();
    }

    virtual int getNetworkSlotId(int slot) const {
        if (slot < 0 || slot > slots.size()) return -1;
        return slot;
    }

    virtual ItemStack* getStackInSlot(int slot) {
        if (slot < 0 || slot >= (int)slots.size()) return nullptr;
        if (!slots[slot].has_value()) return nullptr;
        return &slots[slot].value();
    }

    virtual ItemStack decreaseStackSize(int slot, int count) {
        if (slot < 0 || slot >= (int)slots.size() || !slots[slot].has_value())
            return ItemStack{};
        auto& stack = slots[slot].value();
        if (stack.count <= count) {
            ItemStack taken = stack;
            slots[slot] = std::nullopt;
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
        slots[slot] = stack ? std::optional<ItemStack>(*stack) : std::nullopt;
        onInventoryChanged();
    }

    virtual const std::string& getInventoryName() const { return name; }
    virtual void onInventoryChanged() {}
    virtual bool canInteractWith(EntityPlayer* /*player*/) { return true; }
    virtual ~Inventory() = default;
};
