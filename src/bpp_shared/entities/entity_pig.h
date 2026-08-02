/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_mobile.h"

struct PigEntity : public MobileEntity {
	PigEntity() : MobileEntity() {
		type = EntityType::PIG;
		width = 0.9f;
		height = 0.9f;
		stepHeight = 0.5f;
		maxHealth = 10;
		health = 10;
	}
	~PigEntity() = default;
};