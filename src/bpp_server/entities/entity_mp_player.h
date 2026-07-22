/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "entities.h"
#include "entities/entity_player.h"
#include "inventory/item_stack.h"

struct PlayerSession;
struct EntityMPPlayer : public PlayerEntity {
	PlayerSession* session = nullptr;
	EntityMPPlayer() : PlayerEntity() {
		hasPhysics = false;
		lastHealth = this->GetHeartsHealth();
	}
	~EntityMPPlayer() {
		session = nullptr;
	}
	int lastHealth = 0;
	bool movedThisTick = false;

	void Tick() override;
	bool PickupItem(ItemStack& _stack, EntityId _entityId) override;
	bool DropItem(ItemStack _stack) override;
	void UpdateFallState(float _movedY) override;
	void OnDeath() override;
	void HandlePositionChecks();
	void DropInventory();
};