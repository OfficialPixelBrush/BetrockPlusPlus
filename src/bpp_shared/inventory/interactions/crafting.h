/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once
#include "../inventory_interaction.h"
#include "inventory/inventories.h"
#include "inventory/inventory.h"
#include "runtime.h"

struct CraftingInventoryInteraction : InventoryInteraction {
private:
	Inventory* m_craftInventory;

public:
	InventoryPlayer* m_playerInventory;
	Runtime& m_runtime;

	// We don't do any inventory merging in this class because
	// sharedInventory, craftingInventory, playerInventory all may or may not be the same thing.
	CraftingInventoryInteraction(Inventory* sharedInventory, Inventory* craftingInventory,
	                             InventoryPlayer* playerInventory, Runtime& gameRuntime, UInt8_2 gridSize);

	void onLeftClick(int slot) override;
	void onRightClick(int slot) override;
	void onShiftClick(int slot) override;

protected:
	const UInt8_2 m_gridSize;

	void handleCrafting(int slot);
	void finishCraft();
	void takeResult();

	virtual void updateResult();
	virtual void shiftClickResult() = 0;
	virtual void shiftClickOther(int slot) = 0;
};