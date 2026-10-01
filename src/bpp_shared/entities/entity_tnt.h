/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include "entity.h"
#include "logger.h"
#include "explosion.h"

struct TntEntity : public Entity {
	int fuse = 0;

	TntEntity(Vec3 _pos) : Entity() {
		type = EntityType::LIT_TNT;
		fuse = 80;

		this->Teleport(_pos);

		// Randomness in the direction
		float randomDirection = this->rand.NextDouble() * JavaMath::PI * 2.0;
		velocity.x = -MathHelper::Sin(randomDirection * JavaMath::PI / 180.0) * 0.02;
		velocity.y = 0.2;
		velocity.z = -MathHelper::Cos(randomDirection * JavaMath::PI / 180.0) * 0.02;
	}

	void Tick() override;
	std::optional<Tag> SerializeToNbt() override;
	void LoadFromNbt(Tag& _nbt) override;

	bool CanBeCollidedWith() override {
		return !isDead;
	}

	void Explode() {
		if (!world)
			return;
		world->DoExplosion(this, this->position, /*power=*/4, /*doFire=*/false);
	}
};