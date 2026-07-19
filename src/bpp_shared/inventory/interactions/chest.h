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
	InventoryPlayer* m_playerInventory;
	std::weak_ptr<TileEntityChest> m_chestHandle;
	InventoryChest* m_chestInventory;

	struct SharedInventory : Inventory {
		ChestInventoryInteraction* m_owner = nullptr;
		SharedInventory() : Inventory(63) {}
		void onInventoryChanged() override {
			if (m_owner)
				m_owner->writeBack();
		}
	} m_sharedInventory;

	ChestInventoryInteraction(InventoryPlayer* pinv, std::shared_ptr<TileEntityChest> chest);
	virtual ~ChestInventoryInteraction();

	virtual bool canExist() override;
	void initSnapshot() override;
	std::vector<DeltaSlot> tickDiff() override;
	void mergeInventories();
	void writeBack();
	void onShiftClick(int slot) override;
};