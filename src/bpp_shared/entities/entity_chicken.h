/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_animal.h"

struct ChickenEntity : public AnimalEntity {
	uint32_t ticksUntilEgg = 0;
	ChickenEntity() : AnimalEntity() {
		type = EntityType::CHICKEN;
		width = 0.3f;
		height = 0.4f;
		SetMaxHealth(/*Health=*/4);
	}
	~ChickenEntity() = default;
	void OnDeath(Entity* _killer) override;
	void UpdateFallState(float _movedY) override;
	void RollEggTimer() {
		ticksUntilEgg = this->rand.NextInt(6000) + 6000;
	}
	void Tick() override {
		AnimalEntity::Tick();
		// Chickens can fly
		if (!this->onGround && this->velocity.y < 0.0) {
			this->velocity.y *= 0.6;
		}

		// Lay EGG
		if (ticksUntilEgg-- <= 0) {
			this->DropItemAtEntity(Items::EGG, 1);
			this->RollEggTimer();
		}
	}
};