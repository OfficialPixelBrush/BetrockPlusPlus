/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_mobile.h"

struct PlayerEntity : public MobileEntity {
	PlayerEntity() : MobileEntity() {
		type = EntityType::PLAYER;
		width = 0.6f;
		height = 1.8f;
		stepHeight = 0.5f;
	}
	~PlayerEntity() = default;
	virtual bool pickupItem(ItemStack& _stack, EntityId _entityId);
	virtual bool dropItem(ItemStack _stack);
};