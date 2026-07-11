/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once
#include "../inventory_interaction.h"
#include "inventory/inventories.h"

struct PlayerInventoryInteraction : InventoryInteraction {
	InventoryPlayer* playerInventory;

	PlayerInventoryInteraction(InventoryPlayer* inv);

	bool canExist() override;
	void onShiftClick(int slot) override;
};