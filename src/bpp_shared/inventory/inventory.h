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

// Returned by click() and shiftClick() so the packet handler knows what to send
// without needing to know anything about inventory internals.
enum class ClickResultType {
    Accepted,     
    Rejected,      
    Noop,          
};

struct ClickResult {
    ClickResultType type = ClickResultType::Noop;

    // Which container slots changed and need SetSlot packets.
    // -1 means the cursor slot.
    std::vector<int16_t> changedSlots;

    // If true, the caller should send a full FillContainer instead of
    // individual SetSlot packets (used after shift-clicks).
    bool fullResync = false;
};

// Inventory
struct Inventory {
    std::string name = "Inventory";
    std::vector<std::optional<ItemStack>> slots;

    // Cursor stack — the item held on the mouse during GUI interaction.
    // Lives here so click() can manipulate it alongside slot contents.
    std::optional<ItemStack> carried;

    Inventory(int size) : slots(size) {}

    virtual int getSizeInventory() const {
        return (int)slots.size();
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

    // Normal left/right click on `containerSlot`.
    // `containerSlot` is the raw slot index as sent by the client for this window.
    // Subclasses map it to an inventory slot via slotToInvSlot().
    // Returns a ClickResult the packet handler can act on directly.
    virtual ClickResult click(int16_t containerSlot, bool rightClick) {
        int invSlot = slotToInvSlot(containerSlot);
        if (invSlot < 0) return { ClickResultType::Rejected };

        std::optional<ItemStack> carriedBefore = carried;
        ItemStack* slotStack = getStackInSlot(invSlot);
        std::optional<ItemStack> slotBefore = slotStack
            ? std::optional<ItemStack>(*slotStack) : std::nullopt;

        if (!rightClick) {
            if (!carried && slotStack) {
                carried = *slotStack;
                setInventorySlotContents(invSlot, nullptr);
            } else if (carried && !slotStack) {
                ItemStack copy = *carried;
                setInventorySlotContents(invSlot, &copy);
                carried = std::nullopt;
            } else if (carried && slotStack) {
                if (carried->id == slotStack->id && carried->data == slotStack->data) {
                    int space = GetMaxStack(slotStack->id) - (int)slotStack->count;
                    int move  = (std::min)(space, (int)carried->count);
                    slotStack->count  = (int8_t)(slotStack->count + move);
                    carried->count    = (int8_t)(carried->count - move);
                    if (carried->count <= 0) carried = std::nullopt;
                } else {
                    ItemStack slotCopy    = *slotStack;
                    ItemStack carriedCopy = *carried;
                    setInventorySlotContents(invSlot, &carriedCopy);
                    carried = slotCopy;
                }
            }
        } else {
            if (!carried && slotStack) {
                int half = (slotStack->count + 1) / 2;
                carried = ItemStack{ slotStack->id, (int8_t)half, slotStack->data };
                slotStack->count = (int8_t)(slotStack->count - half);
                if (slotStack->count <= 0) setInventorySlotContents(invSlot, nullptr);
            } else if (carried && !slotStack) {
                ItemStack one{ carried->id, 1, carried->data };
                setInventorySlotContents(invSlot, &one);
                carried->count = (int8_t)(carried->count - 1);
                if (carried->count <= 0) carried = std::nullopt;
            } else if (carried && slotStack) {
                if (carried->id == slotStack->id && carried->data == slotStack->data) {
                    if (slotStack->count < GetMaxStack(slotStack->id)) {
                        slotStack->count  = (int8_t)(slotStack->count + 1);
                        carried->count    = (int8_t)(carried->count - 1);
                        if (carried->count <= 0) carried = std::nullopt;
                    }
                } else {
                    ItemStack slotCopy    = *slotStack;
                    ItemStack carriedCopy = *carried;
                    setInventorySlotContents(invSlot, &carriedCopy);
                    carried = slotCopy;
                }
            }
        }

        // Determine what changed
        bool carriedChanged = (carried != carriedBefore);
        slotStack = getStackInSlot(invSlot);
        std::optional<ItemStack> slotAfter = slotStack
            ? std::optional<ItemStack>(*slotStack) : std::nullopt;
        bool slotChanged = (slotAfter != slotBefore);

        if (!carriedChanged && !slotChanged) return { ClickResultType::Noop };

        ClickResult result;
        result.type = ClickResultType::Accepted;
        if (slotChanged)   result.changedSlots.push_back(containerSlot);
        if (carriedChanged) result.changedSlots.push_back(-1); // cursor
        return result;
    }

    // Click outside the window (slot == -1): drop carried item.
    virtual ClickResult clickOutside(bool rightClick) {
        if (!carried) return { ClickResultType::Noop };
        if (!rightClick) {
            carried = std::nullopt;
        } else {
            carried->count = (int8_t)(carried->count - 1);
            if (carried->count <= 0) carried = std::nullopt;
        }
        return { ClickResultType::Accepted, { -1 } };
    }

    // Shift-click on `containerSlot`. Subclasses override to define where items go
    virtual ClickResult shiftClick(int16_t /*containerSlot*/) {
        return { ClickResultType::Rejected };
    }

    // Maps a container slot index (as the client sends it) to an internal
    // inventory slot index. Returns -1 for invalid or unimplemented slots.
    // Subclasses override this to define their slot layout.
    virtual int slotToInvSlot(int16_t containerSlot) const {
        if (containerSlot < 0 || containerSlot >= (int)slots.size()) return -1;
        return containerSlot;
    }

    // Distribute `working` into inventory slots [rangeStart, rangeEnd).
    // Merges into existing partial stacks first, then fills empty slots.
    // Modifies working.count in place.
    void shiftTransferInto(ItemStack& working, int rangeStart, int rangeEnd, bool reverse) {
        int maxStack = GetMaxStack(working.id);

        // Pass 1: merge into existing partial stacks
        if (maxStack > 1) {
            for (int i = reverse ? rangeEnd - 1 : rangeStart;
                 reverse ? i >= rangeStart : i < rangeEnd;
                 reverse ? --i : ++i) {
                auto* slot = getStackInSlot(i);
                if (!slot || slot->id != working.id || slot->data != working.data) continue;
                int space = maxStack - (int)slot->count;
                if (space <= 0) continue;
                int move = (std::min)(space, (int)working.count);
                slot->count    = (int8_t)(slot->count + move);
                working.count  = (int8_t)(working.count - move);
                if (working.count <= 0) return;
            }
        }

        // Pass 2: fill empty slots
        for (int i = reverse ? rangeEnd - 1 : rangeStart;
             reverse ? i >= rangeStart : i < rangeEnd;
             reverse ? --i : ++i) {
            if (getStackInSlot(i)) continue;
            ItemStack place = working;
            place.count = (int8_t)(std::min((int)working.count, maxStack));
            setInventorySlotContents(i, &place);
            working.count = (int8_t)(working.count - place.count);
            if (working.count <= 0) return;
        }
    }
};
