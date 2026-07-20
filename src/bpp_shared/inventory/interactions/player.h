/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once
#include "entities/entity_player.h"
#include "inventory/interactions/crafting.h"
#include "inventory/inventories.h"

struct PlayerInventoryInteraction : CraftingInventoryInteraction {
	InventoryPlayer* playerInventory;

	PlayerInventoryInteraction(InventoryPlayer* _inv, Runtime& _gameRuntime);
	void onClose();
	bool canExist() override;

protected:
	void shiftClickResult() override;
	void shiftClickOther(int _slot) override;
};