/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include "entity.h"

struct PlayerEntity : public Entity {
	PlayerEntity() : Entity() {
		type = EntityType::PLAYER;
		hasPhysics = false;
		width = 0.6f;
		height = 1.8f;
		stepHeight = 0.5f;
	}
	~PlayerEntity() = default;
	virtual bool pickupItem(ItemStack& stack, EntityId entityId);
};