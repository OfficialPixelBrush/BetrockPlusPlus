/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include "enums/items.h"
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
	int m_activeHotbarSlot = 0;
	int m_currentItem = 0;

	InventoryPlayer() : Inventory(45) {
		m_name = "Inventory";
	}

	ItemStack* getCurrentItem() {
		if (m_currentItem < 0 || m_currentItem >= 9)
			return nullptr;
		return getStackInSlot(m_currentItem);
	}

	ItemStack* getHeldItem() {
		if (m_activeHotbarSlot < 0 || m_activeHotbarSlot >= 9)
			return nullptr;
		return getStackInSlot(m_activeHotbarSlot + 36);
	}

	InvMap getInventoryAreaFromSlot(int slot) {
		if (slot == 0)
			return InvMap::CRAFTING_RESULT;
		if (slot >= 1 && slot <= 4)
			return InvMap::CRAFTING_AREA;
		if (slot >= 5 && slot <= 8)
			return InvMap::ARMOR;
		if (slot >= 36 && slot <= 44)
			return InvMap::HOTBAR;
		if (slot >= 9 && slot <= 35)
			return InvMap::INVENTORY;
		GlobalLogger().m_error << "Invalid Inventory area slot! (" << slot << ")\n";
		return InvMap::INVALID; // Fallback,
	}

	NbtSlotId getNbtSlotID(NetworkSlotId slot) {
		if (slot >= 9 && slot <= 35)
			return slot.m_value;
		if (slot >= 5 && slot <= 8)
			return (5 + (8 - slot)) + 95;
		if (slot >= 36 && slot <= 44)
			return slot - 36;
		return -1;
	}

	NetworkSlotId getNetworkSlotId(NbtSlotId slot) const override {
		if (slot >= 100 && slot <= 103)
			return 5 + (8 - (slot - 95));
		if (slot >= 9 && slot <= 35)
			return slot.m_value;
		if (slot >= 0 && slot <= 8)
			return slot + 36;
		return -1;
	}

	// Tries to "pickup" an item. Returns if it succeeded
	// Not sure why vanilla does it in such a convoluted way
	// but it is the way it is
	bool pickupItem(ItemStack& stack) {
		// Can we combine with anything in the inventory?
		if (canMergeItemStackInInventory(stack, false, 9, 35)) {
			mergeItemStackInInventory(stack, false, 9, 35);
			return true;
		} else {
			// We couldn't combine this stack with anything in the inventory
			// so try the hotbar
			if (mergeItemStackInInventory(stack, false, 36, 44)) {
				return true;
			}
			// Try to find an empty slot in the inventory as a last resort
			return mergeItemStackInInventory(stack, false, 9, 35);
		}
	}

	// Returns whether we could merge an item stack without changing the inventory.
	bool canMergeItemStackInInventory(ItemStack& stack, bool reverse = false, int startSlot = 0, int endSlot = -1) {
		auto start = startSlot;
		auto end = endSlot == -1 ? getSizeInventory() - 1 : endSlot;

		// Try and merge into an already existing stack of the same type if this item is stackable
		if (Items::IsStackable(stack.m_id)) {
			for (int i = reverse ? end : start; reverse ? i >= start : i <= end; reverse ? i-- : i++) {
				auto slot = getStackInSlot(i);
				if (!slot)
					continue;
				if (slot->m_id == stack.m_id && slot->m_data == stack.m_data) {
					auto maxStack = Items::GetMaxStack(slot->m_id);
					// Don't try and merge into an already maxed out stack
					if (slot->m_count >= maxStack)
						continue;

					// Add the stacks together and do some checks to make sure we don't overflow
					int space = maxStack - slot->m_count;
					int toMove = CrossPlatform::Math::min(space, (int)stack.m_count);

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
		m_name = "Chest";
	}
};

// Just a wrapper for two chest inventories
struct InventoryLargeChest : Inventory {
	InventoryChest* m_upper;
	InventoryChest* m_lower;

	InventoryLargeChest(InventoryChest* upper, InventoryChest* lower) : Inventory(0), m_upper(upper), m_lower(lower) {
		m_name = "Large Chest";
	}

	int getSizeInventory() const override {
		return m_upper->getSizeInventory() + m_lower->getSizeInventory();
	}

	ItemStack* getStackInSlot(int slot) override {
		int upperSize = m_upper->getSizeInventory();
		if (slot < upperSize)
			return m_upper->getStackInSlot(slot);
		return m_lower->getStackInSlot(slot - upperSize);
	}

	ItemStack decreaseStackSize(int slot, int count) override {
		int upperSize = m_upper->getSizeInventory();
		if (slot < upperSize)
			return m_upper->decreaseStackSize(slot, count);
		return m_lower->decreaseStackSize(slot - upperSize, count);
	}

	void setInventorySlotContents(int slot, ItemStack* stack) override {
		int upperSize = m_upper->getSizeInventory();
		if (slot < upperSize)
			m_upper->setInventorySlotContents(slot, stack);
		else
			m_lower->setInventorySlotContents(slot - upperSize, stack);
	}

	void onInventoryChanged() override {
		m_upper->onInventoryChanged();
		m_lower->onInventoryChanged();
	}

	bool mergeItemStackInInventory(ItemStack& stack, bool reverse = false, int startSlot = 0,
	                               int endSlot = -1) override {
		int upperSize = m_upper->getSizeInventory();
		int totalSize = upperSize + m_lower->getSizeInventory();
		auto end = endSlot == -1 ? totalSize - 1 : endSlot;

		bool success = m_upper->mergeItemStackInInventory(stack, reverse, CrossPlatform::Math::max(0, startSlot),
		                                                  CrossPlatform::Math::min(upperSize - 1, end));

		if (!success || stack.m_count > 0) {
			success = m_lower->mergeItemStackInInventory(
			    stack, reverse, CrossPlatform::Math::max(0, startSlot - upperSize),
			    CrossPlatform::Math::min(m_lower->getSizeInventory() - 1, end - upperSize));
		}
		return success || stack.m_count == 0;
	}
};

struct InventoryCraftingTable : Inventory {
	InventoryCraftingTable() : Inventory(10) {
		m_name = "Crafting";
	}
};

struct InventoryDispenser : Inventory {
	// TODO: Maybe use JavaRandom? (does that matter???)
	std::mt19937 m_rng{ std::random_device{}() };

	InventoryDispenser() : Inventory(9) {
		m_name = "Trap";
	}

	std::optional<ItemStack> getRandomStack() {
		int chosen = -1, weight = 1;
		for (int i = 0; i < 9; i++) {
			if (m_slots[size_t(i)].m_id == Items::Id::INVALID)
				continue;
			if (std::uniform_int_distribution<int>(0, weight++ - 1)(m_rng) == 0)
				chosen = i;
		}
		if (chosen < 0)
			return std::nullopt;
		return decreaseStackSize(chosen, 1);
	}
};

// TODO: Maybe make an enum for this?
// Slots: 0 = input, 1 = fuel, 2 = output.
struct InventoryFurnace : Inventory {
	int m_burnTime = 0;
	int m_maxBurnTime = 0;
	int m_cookTime = 0;

	InventoryFurnace() : Inventory(3) {
		m_name = "Furnace";
	}
	bool isBurning() const {
		return m_burnTime > 0;
	}
};