/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once
#include "inventories.h"

struct difference {
    std::optional<ItemStack>& stack;
    int slot = 0;
};

// Used for actually interacting with inventories, will typically wrap 1 or more inventory objects for things like chests, etc
struct InventoryInteraction {
    std::vector<std::optional<ItemStack>> snapshot;
    std::optional<ItemStack> carried;
    Inventory* inventory;

    InventoryInteraction(Inventory* inv) : inventory(inv) {}

    virtual bool canExist() {
        return inventory != nullptr;
    }

    // Take a snapshot of this inventory
    virtual void initSnapshot() {
        snapshot = inventory->slots;
    }

    // Analyze the snapshot vs the current inventory
    // Returns a list of slots that are different
    virtual std::vector<difference> tickDiff() {
        std::vector<difference> differences;
        for (size_t i = 0; i < snapshot.size(); i++) {
            auto* current = inventory->getStackInSlot(i);
            auto& snap = snapshot[i];

            bool changed = snap != inventory->slots[i];
            if (!changed) continue;

            snap = inventory->slots[i];
            differences.push_back({ snap, int(i) });
        }
        return differences;
    }

    virtual void onLeftClick(int slot) {
        auto targetSlot = inventory->getStackInSlot(slot);

        // Empty slot
        if (!targetSlot) {
            if (carried.has_value()) {
                inventory->setInventorySlotContents(slot, &carried.value());
                carried = std::nullopt;
            }
            inventory->onInventoryChanged();
            return;
        }

        // Not carrying anything
        if (!carried.has_value()) {
            carried = *targetSlot;
            inventory->clearSlot(slot);
            inventory->onInventoryChanged();
            return;
        }

        // Same item; merge
        if (targetSlot->id == carried->id && targetSlot->data == carried->data) {
            int maxStack = GetMaxStack(targetSlot->id);
            int space = maxStack - targetSlot->count;
            int toMove = std::min(space, (int)carried->count);
            targetSlot->count += toMove;
            carried->count -= toMove;
            if (carried->count == 0) carried = std::nullopt;
            inventory->onInventoryChanged();
            return;
        }

        // Different item; swap
        ItemStack temp = *targetSlot;
        *targetSlot = *carried;
        carried = temp;
        inventory->onInventoryChanged();
    }

    virtual void onRightClick(int slot) {
        auto targetSlot = inventory->getStackInSlot(slot);

        if (carried.has_value()) {
            if (!targetSlot) {
                ItemStack single{ carried->id, 1, carried->data };
                inventory->setInventorySlotContents(slot, &single);
                carried->count -= 1;
                if (carried->count == 0) carried = std::nullopt;
                inventory->onInventoryChanged();
                return;
            }

            // If we right click on the same item we are carrying just add one
            if (targetSlot->id == carried->id && targetSlot->data == carried->data) {
                int maxStack = GetMaxStack(targetSlot->id);
                int space = maxStack - targetSlot->count;
                if (space >= 1) {
                    targetSlot->count += 1;
                    carried->count -= 1;
                    if (carried->count == 0) carried = std::nullopt;
                    inventory->onInventoryChanged();
                }
                return;
            }

            // If we right click on a different item, swap the cursor and that item
            ItemStack temp = *targetSlot;
            *targetSlot = *carried;
            carried = temp;
            inventory->onInventoryChanged();
            return;
        }

        if (!targetSlot) return;

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

    virtual void onShiftClick(int slot) {
        auto targetSlot = inventory->getStackInSlot(slot);
        if (!targetSlot) return;
        inventory->mergeItemStackInInventory(*targetSlot);
    }
};

struct LargeChestInventoryInteraction : InventoryInteraction {
    InventoryPlayer* playerInventory;
    std::weak_ptr<TileEntityChest> upperChest;
    std::weak_ptr<TileEntityChest> lowerChest;
    InventoryLargeChest chestInventory;

    struct SharedInventory : Inventory {
        LargeChestInventoryInteraction* owner = nullptr;
        SharedInventory() : Inventory(90) {}
        void onInventoryChanged() override {
            if (owner) owner->writeBack();
        }
    } sharedInventory;

    LargeChestInventoryInteraction(InventoryPlayer* pinv, std::shared_ptr<TileEntityChest> upper, std::shared_ptr<TileEntityChest> lower)
        : InventoryInteraction(&sharedInventory), playerInventory(pinv),
        upperChest(upper), lowerChest(lower),
        chestInventory(&upper->inventory, &lower->inventory) {
        sharedInventory.owner = this;
        mergeInventories();
    }

    ~LargeChestInventoryInteraction() {
        if (canExist()) writeBack();
    }

    virtual bool canExist() {
        return !upperChest.expired() && !lowerChest.expired();
    }

    // Only snapshot the chest portion we don't need the attached inventory
    void initSnapshot() override {
        snapshot.resize(size_t(chestInventory.getSizeInventory()));
        for (size_t i = 0; i < size_t(chestInventory.getSizeInventory()); i++) {
            auto* stack = chestInventory.getStackInSlot(i);
            snapshot[i] = stack ? std::optional<ItemStack>(*stack) : std::nullopt;
        }
    }

    // Analyze the snapshot vs the current chest inventory
    std::vector<difference> tickDiff() override {
        std::vector<difference> differences;
        for (size_t i = 0; i < snapshot.size(); i++) {
            auto* current = chestInventory.getStackInSlot(int(i));
            auto currentOpt = current ? std::optional<ItemStack>(*current) : std::nullopt;

            if (snapshot[i] == currentOpt) continue;

            snapshot[i] = currentOpt;
            differences.push_back({ snapshot[i], int(i) });
        }
        mergeInventories();
        return differences;
    }

    void mergeInventories() {
        size_t slotCount = 0;
        for (size_t i = 0; i < size_t(chestInventory.getSizeInventory()); i++) {
            auto* stack = chestInventory.getStackInSlot(i);
            sharedInventory.slots[slotCount++] = stack ? std::optional<ItemStack>(*stack) : std::nullopt;
        }
        for (size_t i = 9; i <= 44; i++)
            sharedInventory.slots[slotCount++] = playerInventory->slots[i];
    }

    void writeBack() {
        for (size_t i = 0; i < 54; i++) {
            auto& slot = sharedInventory.slots[i];
            ItemStack* ptr = slot.has_value() ? &slot.value() : nullptr;
            chestInventory.setInventorySlotContents(i, ptr);
        }
        for (size_t i = 54; i < 90; i++)
            playerInventory->slots[i - 54 + 9] = sharedInventory.slots[i];
    }

    void onShiftClick(int slot) override {
        auto stack = sharedInventory.getStackInSlot(slot);
        if (!stack) return;

        ItemStack copy = *stack;

        if (slot <= 53) {
            // Chest -> inventory
            bool success = playerInventory->mergeItemStackInInventory(copy, true, 9, 44);
        }
        else {
            // Inventory -> Chest
            bool success = chestInventory.mergeItemStackInInventory(copy);
        }

        // Update the source in the real inventory before re-merging
        if (slot <= 53) {
            ItemStack* ptr = copy.count == 0 ? nullptr : &copy;
            chestInventory.setInventorySlotContents(slot, ptr);
        }
        else {
            int playerSlot = slot - 54 + 9;
            playerInventory->slots[size_t(playerSlot)] = copy.count == 0 ? std::nullopt : std::optional<ItemStack>(copy);
        }

        // Re-sync sharedInventory from the real inventories
        mergeInventories();
    }
};

struct ChestInventoryInteraction : InventoryInteraction {
    InventoryPlayer* playerInventory;
    std::weak_ptr<TileEntityChest> chestHandle;
    InventoryChest* chestInventory;

    struct SharedInventory : Inventory {
        ChestInventoryInteraction* owner = nullptr;
        SharedInventory() : Inventory(63) {}
        void onInventoryChanged() override {
            if (owner) owner->writeBack();
        }
    } sharedInventory;

    ChestInventoryInteraction(InventoryPlayer* pinv, std::shared_ptr<TileEntityChest> chest)
        : InventoryInteraction(&sharedInventory), playerInventory(pinv),
        chestHandle(chest), chestInventory(&chest->inventory) {
        sharedInventory.owner = this;
        mergeInventories();
    }

    ~ChestInventoryInteraction() {
        if (canExist()) writeBack();
    }

    virtual bool canExist() {
        return !chestHandle.expired();
    }

    // Only snapshot the chest portion we don't need the attached inventory
    void initSnapshot() override {
        snapshot.resize(size_t(chestInventory->getSizeInventory()));
        for (size_t i = 0; i < chestInventory->getSizeInventory(); i++)
            snapshot[i] = chestInventory->slots[i];
    }

    // Analyze the snapshot vs the current chest inventory
    std::vector<difference> tickDiff() {
        std::vector<difference> differences;
        for (size_t i = 0; i < snapshot.size(); i++) {
            auto* current = chestInventory->getStackInSlot(i);
            auto& snap = snapshot[i];

            bool changed = snap != chestInventory->slots[i];
            if (!changed) continue;

            snap = chestInventory->slots[i];
            differences.push_back({ snap, int(i) });
        }
        mergeInventories();
        return differences;
    }

    void mergeInventories() {
        size_t slotCount = 0;
        for (auto& slot : chestInventory->slots)
            sharedInventory.slots[slotCount++] = slot;
        for (size_t i = 9; i <= 44; i++)
            sharedInventory.slots[slotCount++] = playerInventory->slots[i];
    }

    void writeBack() {
        for (size_t i = 0; i < 27; i++)
            chestInventory->slots[i] = sharedInventory.slots[i];
        for (size_t i = 27; i < 63; i++)
            playerInventory->slots[i - 27 + 9] = sharedInventory.slots[i];
    }

    void onShiftClick(int slot) override {
        auto stack = sharedInventory.getStackInSlot(slot);
        if (!stack) return;

        ItemStack copy = *stack;

        if (slot <= 26) {
            // Chest -> inventory
            bool success = playerInventory->mergeItemStackInInventory(copy, true, 9, 44);
        }
        else {
            // Inventory -> Chest
            bool success = chestInventory->mergeItemStackInInventory(copy);
        }

        // Update the source in the real inventory before re-merging
        if (slot <= 26) {
            chestInventory->slots[size_t(slot)] = copy.count == 0 ? std::nullopt : std::optional<ItemStack>(copy);
        }
        else {
            int playerSlot = slot - 27 + 9;
            playerInventory->slots[size_t(playerSlot)] = copy.count == 0 ? std::nullopt : std::optional<ItemStack>(copy);
        }

        // Re-sync sharedInventory from the real inventories
        mergeInventories();
    }
};

struct PlayerInventoryInteraction : InventoryInteraction {
    InventoryPlayer* playerInventory;

    PlayerInventoryInteraction(InventoryPlayer* inv)
        : InventoryInteraction(inv), playerInventory(inv) {
    }

    virtual bool canExist() {
        return playerInventory != nullptr;
    }

    void onShiftClick(int slot) override {
        auto from = playerInventory->getInventoryAreaFromSlot(slot);
        auto stack = playerInventory->getStackInSlot(slot);
        if (!stack) return;

        ItemStack copy = *stack;  // work on a copy so we can detect what moved

        if (from == invMap::armor || from == invMap::crafting || from == invMap::craftingResult || from == invMap::hotbar) {
            bool success = playerInventory->mergeItemStackInInventory(copy, false, 9, 35);
            if (!success) playerInventory->mergeItemStackInInventory(copy, false, 36, 44);
        }
        else {
            playerInventory->mergeItemStackInInventory(copy, false, 36, 44);
        }

        // Update the source slot to whatever count is left
        if (copy.count == 0) {
            playerInventory->clearSlot(slot);
        }
        else {
            stack->count = copy.count;
        }
    }
};