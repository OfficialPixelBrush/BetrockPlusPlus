/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_animal.h"

struct SheepEntity : public AnimalEntity {
	int8_t color = 0;
	bool sheared = false;
	
	SheepEntity() : AnimalEntity() {
		type = EntityType::SHEEP;
		width = 0.9f;
		height = 1.3f;
		SetMaxHealth(/*Health=*/8);
	}
	~SheepEntity() = default;
	void OnDeath() override;
};