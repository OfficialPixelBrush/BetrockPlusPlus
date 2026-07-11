/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once
#include "../inventory_interaction.h"
#include "inventory/inventories.h"
#include "runtime.h"
#include "world.h"

struct CraftingInventoryInteraction : InventoryInteraction {
	InventoryPlayer* playerInventory;
	InventoryCrafting craftInventory;
	WorldManager& world;
	Runtime& runtime;
	Int3 blockPosition;
	ItemStack lastResult;

	struct SharedInventory : Inventory {
		CraftingInventoryInteraction* owner = nullptr;
		SharedInventory() : Inventory(46) {}
		void onInventoryChanged() override {
			if (owner)
				owner->writeBack();
		}
	} sharedInventory;

	CraftingInventoryInteraction(InventoryPlayer* pinv, WorldManager& worldMng, Runtime& gameRuntime,
	                             Int3 craftingTablePos);

	virtual ~CraftingInventoryInteraction();

	bool canExist() override;
	void initSnapshot() override;
	std::vector<DeltaSlot> tickDiff() override;
	void onLeftClick(int slot) override;
	void onRightClick(int slot) override;
	void onShiftClick(int slot) override;

private:
	void mergeInventories();
	void writeBack();
	void updateResult();
	void handleCrafting(int slot);
	void finishCraft();
	void takeResult();
	void shiftClickResult();
	void shiftClickGrid(int slot);
};