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
	void OnClose();
	bool CanExist() override;

protected:
	void ShiftClickResult() override;
	void ShiftClickOther(int _slot) override;
};