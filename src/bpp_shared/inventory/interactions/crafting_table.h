/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once
#include "../inventory_interaction.h"
#include "inventory/interactions/crafting.h"
#include "inventory/inventories.h"
#include "runtime.h"
#include "world.h"

struct CraftingTableInventoryInteraction : CraftingInventoryInteraction {
	InventoryCraftingTable m_craftInventory;
	WorldManager& m_world;
	Int3 m_blockPosition;

	struct SharedInventory : Inventory {
		CraftingTableInventoryInteraction* owner = nullptr;
		SharedInventory() : Inventory(46) {}
		void onInventoryChanged() override {
			if (owner)
				owner->writeBack();
		}
	} m_sharedInventory;

	CraftingTableInventoryInteraction(InventoryPlayer* pinv, WorldManager& worldMng, Runtime& gameRuntime,
	                                  Int3 craftingTablePos);
	~CraftingTableInventoryInteraction();

	bool canExist() override;
	void initSnapshot() override;
	std::vector<DeltaSlot> tickDiff() override;
	void writeBack();

protected:
	void mergeInventories();
	void updateResult() override;
	void shiftClickResult() override;
	void shiftClickOther(int slot) override;
};