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
	PlayerSession* m_session = nullptr;
	EntityMPPlayer() : PlayerEntity() {
		m_hasPhysics = false;
	}
	~EntityMPPlayer() {
		m_session = nullptr;
	}
	void tick() override;
	bool pickupItem(ItemStack& stack, EntityId entityId) override;
	bool dropItem(ItemStack stack) override;
	void handlePositionChecks();
};