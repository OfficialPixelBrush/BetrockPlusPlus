/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_animal.h"

struct SquidEntity : public AnimalEntity {
	Vec3 randomVelocity = {};
	float randomMotionSpeed = 0.0f;
	float rotationVelocity = 0.0f;
	float rotation = 0.0f;

	SquidEntity() : AnimalEntity() {
		type = EntityType::SQUID;
		canBreatheUnderwater = true;
		rotationVelocity = 1.0f / (rand.NextFloat() + 1.0f) * 0.2f;
		SetSize({ 0.95, 0.95 });
	}
	~SquidEntity() = default;
	void UpdateAIState() override;
	bool CanSpawnAt() override;
	void OnDeath(Entity* _killer) override;
	void Tick() override;
	void TickPhysics() override {
		Move(this->velocity);
	}
	bool IsSwimming() {
		if (!this->world)
			return false;
		return this->world->HandleFluidAcceleration(collider.Expand(0.0, -0.6, 0.0), Material::Water(), *this);
	}
};