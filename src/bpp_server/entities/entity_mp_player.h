/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "entities.h"
#include "entities/entity_player.h"

struct PlayerSession;
struct EntityMPPlayer : public PlayerEntity {
	PlayerSession* session = nullptr;
	EntityMPPlayer() : PlayerEntity() {}
	~EntityMPPlayer() {
		session = nullptr;
	}
	void tick() override;
	bool pickupItem(ItemStack& stack, EntityId entityId) override;
};