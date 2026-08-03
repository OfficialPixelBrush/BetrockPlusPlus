/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_animal.h"

struct CowEntity : public AnimalEntity {
	CowEntity() : AnimalEntity() {
		type = EntityType::COW;
		width = 0.9f;
		height = 1.3f;
	}
	~CowEntity() = default;
	void OnDeath() override;
};