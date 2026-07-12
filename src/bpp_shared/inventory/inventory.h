/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once
#include "cross_platform.h"
#include "enums/items.h"
#include "item_stack.h"
#include <cstdint>
#include <string>
#include <vector>

// Inventory
struct Inventory {
	std::string name = "Inventory";
	std::vector<ItemStack> slots;
	bool isModified = false;

	Inventory(size_t size) : slots(size) {}

	virtual int getSizeInventory() const {
		return int(slots.size());
	}

	virtual NetworkSlotId getNetworkSlotId(NbtSlotId slot) const {
		if (slot < 0 || slot >= int(slots.size()))
			return -1;
		return slot.m_value;
	}

	virtual ItemStack* getStackInSlot(int slot) {
		if (slot < 0 || slot >= (int)slots.size())
			return nullptr;
		if (slots[size_t(slot)].id == ITEM_INVALID)
			return nullptr;
		return &slots[size_t(slot)];
	}

	virtual ItemStack decreaseStackSize(int slot, int count) {
		if (slot < 0 || slot >= (int)slots.size() || slots[size_t(slot)].id == ITEM_INVALID)
			return ItemStack{};
		auto& stack = slots[size_t(slot)];
		if (stack.count <= count) {
			ItemStack taken = stack;
			slots[slot] = ItemStack{};
			onInventoryChanged();
			return taken;
		}
		ItemStack taken{ stack.id, (int8_t)count, stack.data };
		stack.count = (int8_t)(stack.count - count);
		onInventoryChanged();
		return taken;
	}

	virtual void setInventorySlotContents(int slot, ItemStack* stack) {
		if (slot < 0 || slot >= (int)slots.size())
			return;
		slots[size_t(slot)] = stack ? *stack : ItemStack{};
		onInventoryChanged();
	}

	virtual const std::string& getInventoryName() const {
		return name;
	}
	virtual void onInventoryChanged() {
		isModified = true;
	}
	virtual ~Inventory() = default;

	void clearSlot(int slot) {
		if (slot < 0 || slot >= (int)slots.size())
			return;
		slots[size_t(slot)] = ItemStack{};
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
				if (!slot)
					continue;
				if (slot->id == stack.id && slot->data == stack.data) {
					auto maxStack = GetMaxStack(slot->id);
					// Don't try and merge into an already maxed out stack
					if (slot->count >= maxStack)
						continue;

					// Add the stacks together and do some checks to make sure we don't overflow
					int space = maxStack - slot->count;
					int toMove = CrossPlatform::Math::min(space, (int)stack.count);

					slot->count += toMove;
					stack.count -= toMove;

					onInventoryChanged();
					if (stack.count == 0)
						return true;
				}
			}
		}

		// We couldn't merge into existing items so just try and find an empty slot
		for (int i = reverse ? end : start; reverse ? i >= start : i <= end; reverse ? i-- : i++) {
			if (slots[size_t(i)].id == ITEM_INVALID) {
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
