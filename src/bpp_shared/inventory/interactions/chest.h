/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once
#include "../inventory_interaction.h"
#include "inventory/inventories.h"
#include "tile_entities/tile_entity.h"

struct ChestInventoryInteraction : InventoryInteraction {
	InventoryPlayer* playerInventory;
	std::weak_ptr<TileEntityChest> chestHandle;
	InventoryChest* chestInventory;

	struct SharedInventory : Inventory {
		ChestInventoryInteraction* owner = nullptr;
		SharedInventory() : Inventory(63) {}
		void OnInventoryChanged() override {
			if (owner)
				owner->WriteBack();
		}
	} sharedInventory;

	ChestInventoryInteraction(InventoryPlayer* _pinv, std::shared_ptr<TileEntityChest> _chest);
	virtual ~ChestInventoryInteraction();

	virtual bool CanExist() override;
	void InitSnapshot() override;
	std::vector<DeltaSlot> TickDiff() override;
	void MergeInventories();
	void WriteBack();
	void OnShiftClick(int _slot) override;
};