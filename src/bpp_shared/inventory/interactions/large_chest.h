/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once
#include "../inventory_interaction.h"
#include "inventory/inventories.h"
#include "tile_entities/tile_entity.h"

struct LargeChestInventoryInteraction : InventoryInteraction {
	InventoryPlayer* playerInventory;
	std::weak_ptr<TileEntityChest> upperChest;
	std::weak_ptr<TileEntityChest> lowerChest;
	InventoryLargeChest chestInventory;

	struct SharedInventory : Inventory {
		LargeChestInventoryInteraction* owner = nullptr;
		SharedInventory() : Inventory(90) {}
		void onInventoryChanged() override {
			if (owner)
				owner->writeBack();
		}
	} sharedInventory;

	LargeChestInventoryInteraction(InventoryPlayer* pinv, std::shared_ptr<TileEntityChest> upper,
	                               std::shared_ptr<TileEntityChest> lower);
	virtual ~LargeChestInventoryInteraction();

	bool canExist() override;
	void initSnapshot() override;
	std::vector<DeltaSlot> tickDiff() override;
	void mergeInventories();
	void writeBack();
	void onShiftClick(int slot) override;
};