/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once
#include "inventory.h"
#include "inventory/item_stack.h"

struct DeltaSlot {
	ItemStack stack;
	int slot = 0;
};

// Used for actually interacting with inventories, will typically wrap 1 or more inventory objects for things like chests, etc
struct InventoryInteraction {
	std::vector<ItemStack> snapshot;
	ItemStack carried;
	Inventory* inventory;

	InventoryInteraction(Inventory* _inv);
	virtual ~InventoryInteraction() = default;

	virtual bool canExist();

	// Take a snapshot of this inventory
	virtual void initSnapshot();

	// Analyze the snapshot vs the current inventory
	// Returns a list of slots that are different
	virtual std::vector<DeltaSlot> tickDiff();

	virtual void onLeftClick(int _slot);

	virtual void onRightClick(int _slot);

	virtual void onShiftClick(int _slot);
};