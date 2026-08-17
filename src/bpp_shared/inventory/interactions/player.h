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
	bool needsDiff = false;

	PlayerInventoryInteraction(InventoryPlayer* _inv, Runtime& _gameRuntime);
	void OnClose();
	bool CanExist(PlayerEntity& player) override;
	void OnLeftClick(int _slot) override;
	void OnRightClick(int _slot) override;
protected:
	void ShiftClickResult() override;
	void ShiftClickOther(int _slot) override;
};