/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_animal.h"

struct ChickenEntity : public AnimalEntity {
	ChickenEntity() : AnimalEntity() {
		type = EntityType::CHICKEN;
		width = 0.3f;
		height = 0.4f;
		SetMaxHealth(/*Health=*/4);
	}
	~ChickenEntity() = default;
	void OnDeath(Entity* _killer) override;
	void UpdateFallState(float _movedY) override;
	void Tick() override {
		AnimalEntity::Tick();
		// Chickens can fly
		if (!this->onGround && this->velocity.y < 0.0) {
			this->velocity.y *= 0.6;
		}
	}
};