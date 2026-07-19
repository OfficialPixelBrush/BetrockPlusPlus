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
		m_type = EntityType::PLAYER;
		m_hasPhysics = false;
		m_width = 0.6f;
		m_height = 1.8f;
		m_stepHeight = 0.5f;
	}
	~PlayerEntity() = default;
	virtual bool pickupItem(ItemStack& stack, EntityId entityId);
	virtual bool dropItem(ItemStack stack);
};