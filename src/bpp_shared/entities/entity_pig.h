/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_animal.h"

struct PigEntity : public AnimalEntity {
	PigEntity() : AnimalEntity() {
		type = EntityType::PIG;
		width = 0.9f;
		height = 0.9f;
	}
	~PigEntity() = default;
	void OnDeath(Entity* _killer) override;
};