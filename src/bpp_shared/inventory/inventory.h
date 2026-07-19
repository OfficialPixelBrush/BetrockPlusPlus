/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once
#include "cross_platform.h"
#include "enums/items.h"
#include "item_stack.h"
#include "items/item_properties.h"
#include <cstdint>
#include <string>
#include <vector>

// Inventory
struct Inventory {
	std::string m_name = "Inventory";
	std::vector<ItemStack> m_slots;
	bool m_isModified = false;

	Inventory(size_t size) : m_slots(size) {}

	virtual int getSizeInventory() const {
		return int(m_slots.size());
	}

	virtual NetworkSlotId getNetworkSlotId(NbtSlotId slot) const {
		if (slot < 0 || slot >= int(m_slots.size()))
			return -1;
		return slot.m_value;
	}

	virtual ItemStack* getStackInSlot(int slot) {
		if (slot < 0 || slot >= (int)m_slots.size())
			return nullptr;
		if (m_slots[size_t(slot)].m_id == Items::Id::INVALID)
			return nullptr;
		return &m_slots[size_t(slot)];
	}

	virtual ItemStack decreaseStackSize(int slot, int count) {
		if (slot < 0 || slot >= (int)m_slots.size() || m_slots[size_t(slot)].m_id == Items::Id::INVALID)
			return ItemStack{};
		auto& stack = m_slots[size_t(slot)];
		if (stack.m_count <= count) {
			ItemStack taken = stack;
			m_slots[slot] = ItemStack{};
			onInventoryChanged();
			return taken;
		}
		ItemStack taken{ stack.m_id, (int8_t)count, stack.m_data };
		stack.m_count = (int8_t)(stack.m_count - count);
		onInventoryChanged();
		return taken;
	}

	virtual void setInventorySlotContents(int slot, ItemStack* stack) {
		if (slot < 0 || slot >= (int)m_slots.size())
			return;
		m_slots[size_t(slot)] = stack ? *stack : ItemStack{};
		onInventoryChanged();
	}

	virtual const std::string& getInventoryName() const {
		return m_name;
	}
	virtual void onInventoryChanged() {
		m_isModified = true;
	}
	virtual ~Inventory() = default;

	void clearSlot(int slot) {
		if (slot < 0 || slot >= (int)m_slots.size())
			return;
		m_slots[size_t(slot)] = ItemStack{};
		onInventoryChanged();
	}

	// Take in the original item stack, try and merge it with our inventory. Returns if it was successful
	// Start slot and end slot are inclusive
	virtual bool mergeItemStackInInventory(ItemStack& stack, bool reverse = false, int startSlot = 0, int endSlot = -1) {
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

					slot->m_count += toMove;
					stack.m_count -= toMove;

					onInventoryChanged();
					if (stack.m_count == 0)
						return true;
				}
			}
		}

		// We couldn't merge into existing items so just try and find an empty slot
		for (int i = reverse ? end : start; reverse ? i >= start : i <= end; reverse ? i-- : i++) {
			if (m_slots[size_t(i)].m_id == Items::Id::INVALID) {
				m_slots[size_t(i)] = ItemStack{ stack.m_id, stack.m_count, stack.m_data };
				stack.m_id = Items::Id::INVALID;
				stack.m_data = 0;
				stack.m_count = 0;
				onInventoryChanged();
				return true;
			}
		}
		return false; // Give up
	}
};
