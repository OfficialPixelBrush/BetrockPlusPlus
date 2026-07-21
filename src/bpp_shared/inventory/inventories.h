/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include "enums/items.h"
#include "events.h"
#include "inventory.h"
#include "item_stack.h"
#include "logger.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <random>

enum InvMap {
	ARMOR,
	INVENTORY,
	HOTBAR,
	CRAFTING_AREA,
	CRAFTING_RESULT,
	INVALID
};

// Network format (The rest of the inventories are self explanatory this is the only one that is semi-convoluted):
// Slots 5 -> 8 are for armor
// Slots 36 -> 44 are the hotbar
// Slots 9 -> 35 are the main inventory
// Slots 1 -> 4 are the crafting grid
// Slot 0 is the crafting result
struct InventoryPlayer : Inventory {
public:
	int activeHotbarSlot = 0;
	Event<ItemStack*> heldItemUpdate;
	int currentItem = 0;

	InventoryPlayer() : Inventory(45) {
		name = "Inventory";
	}

	void SetHeldItem(ItemStack* _stack) {
		heldItemUpdate.Call(_stack);
	}

	ItemStack* GetCurrentItem() {
		if (currentItem < 0 || currentItem >= 9)
			return nullptr;
		return GetStackInSlot(currentItem);
	}

	ItemStack* GetHeldItem() {
		if (activeHotbarSlot < 0 || activeHotbarSlot >= 9)
			return nullptr;
		return GetStackInSlot(activeHotbarSlot + 36);
	}

	InvMap GetInventoryAreaFromSlot(int _slot) {
		if (_slot == 0)
			return InvMap::CRAFTING_RESULT;
		if (_slot >= 1 && _slot <= 4)
			return InvMap::CRAFTING_AREA;
		if (_slot >= 5 && _slot <= 8)
			return InvMap::ARMOR;
		if (_slot >= 36 && _slot <= 44)
			return InvMap::HOTBAR;
		if (_slot >= 9 && _slot <= 35)
			return InvMap::INVENTORY;
		GlobalLogger().error << "Invalid Inventory area slot! (" << _slot << ")\n";
		return InvMap::INVALID; // Fallback,
	}

	NbtSlotId GetNbtSlotId(NetworkSlotId _slot) {
		if (_slot >= 9 && _slot <= 35)
			return _slot.value;
		if (_slot >= 5 && _slot <= 8)
			return (5 + (8 - _slot)) + 95;
		if (_slot >= 36 && _slot <= 44)
			return _slot - 36;
		return -1;
	}

	NetworkSlotId GetNetworkSlotId(NbtSlotId _slot) const override {
		if (_slot >= 100 && _slot <= 103)
			return 5 + (8 - (_slot - 95));
		if (_slot >= 9 && _slot <= 35)
			return _slot.value;
		if (_slot >= 0 && _slot <= 8)
			return _slot + 36;
		return -1;
	}

	// Tries to "pickup" an item. Returns if it succeeded
	// Not sure why vanilla does it in such a convoluted way
	// but it is the way it is
	bool PickupItem(ItemStack& _stack) {
		// Can we combine with anything in the inventory?
		if (CanMergeItemStackInInventory(_stack, false, 9, 35)) {
			MergeItemStackInInventory(_stack, false, 9, 35);
			return true;
		} else {
			// We couldn't combine this stack with anything in the inventory
			// so try the hotbar
			if (MergeItemStackInInventory(_stack, false, 36, 44)) {
				return true;
			}
			// Try to find an empty slot in the inventory as a last resort
			return MergeItemStackInInventory(_stack, false, 9, 35);
		}
	}

	// Returns whether we could merge an item stack without changing the inventory.
	bool CanMergeItemStackInInventory(ItemStack& _stack, bool _reverse = false, int _startSlot = 0, int _endSlot = -1) {
		auto start = _startSlot;
		auto end = _endSlot == -1 ? GetSizeInventory() - 1 : _endSlot;

		// Try and merge into an already existing stack of the same type if this item is stackable
		if (Items::IsStackable(_stack.id)) {
			for (int i = _reverse ? end : start; _reverse ? i >= start : i <= end; _reverse ? i-- : i++) {
				auto slot = GetStackInSlot(i);
				if (!slot)
					continue;
				if (slot->id == _stack.id && slot->data == _stack.data) {
					auto maxStack = Items::GetMaxStack(slot->id);
					// Don't try and merge into an already maxed out stack
					if (slot->count >= maxStack)
						continue;

					// Add the stacks together and do some checks to make sure we don't overflow
					int space = maxStack - slot->count;
					int toMove = CrossPlatform::Math::Min(space, (int)_stack.count);

					if (toMove > 0)
						return true;
				}
			}
		}
		return false; // Give up
	}
};

struct InventoryChest : Inventory {
	InventoryChest() : Inventory(27) {
		name = "Chest";
	}
};

// Just a wrapper for two chest inventories
struct InventoryLargeChest : Inventory {
	InventoryChest* upper;
	InventoryChest* lower;

	InventoryLargeChest(InventoryChest* _upper, InventoryChest* _lower) : Inventory(0), upper(_upper), lower(_lower) {
		name = "Large Chest";
	}

	int GetSizeInventory() const override {
		return upper->GetSizeInventory() + lower->GetSizeInventory();
	}

	ItemStack* GetStackInSlot(int _slot) override {
		int upperSize = upper->GetSizeInventory();
		if (_slot < upperSize)
			return upper->GetStackInSlot(_slot);
		return lower->GetStackInSlot(_slot - upperSize);
	}

	ItemStack DecreaseStackSize(int _slot, int _count) override {
		int upperSize = upper->GetSizeInventory();
		if (_slot < upperSize)
			return upper->DecreaseStackSize(_slot, _count);
		return lower->DecreaseStackSize(_slot - upperSize, _count);
	}

	void SetInventorySlotContents(int _slot, ItemStack* _stack) override {
		int upperSize = upper->GetSizeInventory();
		if (_slot < upperSize)
			upper->SetInventorySlotContents(_slot, _stack);
		else
			lower->SetInventorySlotContents(_slot - upperSize, _stack);
	}

	void OnInventoryChanged() override {
		upper->OnInventoryChanged();
		lower->OnInventoryChanged();
	}

	bool MergeItemStackInInventory(ItemStack& _stack, bool _reverse = false, int _startSlot = 0,
	                               int _endSlot = -1) override {
		int upperSize = upper->GetSizeInventory();
		int totalSize = upperSize + lower->GetSizeInventory();
		auto end = _endSlot == -1 ? totalSize - 1 : _endSlot;

		bool success = upper->MergeItemStackInInventory(_stack, _reverse, CrossPlatform::Math::Max(0, _startSlot),
		                                                CrossPlatform::Math::Min(upperSize - 1, end));

		if (!success || _stack.count > 0) {
			success = lower->MergeItemStackInInventory(
			    _stack, _reverse, CrossPlatform::Math::Max(0, _startSlot - upperSize),
			    CrossPlatform::Math::Min(lower->GetSizeInventory() - 1, end - upperSize));
		}
		return success || _stack.count == 0;
	}
};

struct InventoryCraftingTable : Inventory {
	InventoryCraftingTable() : Inventory(10) {
		name = "Crafting";
	}
};

struct InventoryDispenser : Inventory {
	// TODO: Maybe use JavaRandom? (does that matter???)
	std::mt19937 rng{ std::random_device{}() };

	InventoryDispenser() : Inventory(9) {
		name = "Trap";
	}

	std::optional<ItemStack> GetRandomStack() {
		int chosen = -1, weight = 1;
		for (int i = 0; i < 9; i++) {
			if (slots[size_t(i)].id == Items::Id::INVALID)
				continue;
			if (std::uniform_int_distribution<int>(0, weight++ - 1)(rng) == 0)
				chosen = i;
		}
		if (chosen < 0)
			return std::nullopt;
		return DecreaseStackSize(chosen, 1);
	}
};

// TODO: Maybe make an enum for this?
// Slots: 0 = input, 1 = fuel, 2 = output.
struct InventoryFurnace : Inventory {
	int burnTime = 0;
	int maxBurnTime = 0;
	int cookTime = 0;

	InventoryFurnace() : Inventory(3) {
		name = "Furnace";
	}
	bool IsBurning() const {
		return burnTime > 0;
	}
};