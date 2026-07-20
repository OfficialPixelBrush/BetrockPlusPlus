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
	}
	~EntityMPPlayer() {
		session = nullptr;
	}
	void Tick() override;
	bool PickupItem(ItemStack& _stack, EntityId _entityId) override;
	bool DropItem(ItemStack _stack) override;
	void HandlePositionChecks();
};