/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_mobile.h"

struct MobEntity : public MobileEntity {
	std::weak_ptr<Entity> target;
	TickTime ticksToFollowTarget = 0;
	float randomYawVelocity = 0.0f;

	MobEntity() : MobileEntity() {
		width = 0.9f;
		height = 0.9f;
		stepHeight = 0.5f;
		SetMaxHealth(/*Health=*/10);
	}
	~MobEntity() = default;
	Vec2 FaceEntity(MobileEntity& _entity, float _maxYaw, float _maxPitch);
	virtual void Wander();
	virtual float GetWanderWeight(Int3 _pos);
	virtual void UpdateState();
	virtual void OnDeath() override;
	virtual void Tick() override {
		MobileEntity::Tick();

		randomYawVelocity *= 0.9f;

		if (EntityAlive()) {
			Wander();
			FollowPath();
		}

		TryDespawn();
	}
	virtual bool CanSpawnAt(Int3 _pos) {
		if (!world)
			return false;
		return MobileEntity::CanSpawnAt(_pos);
	}
	virtual bool TryDespawn() {
		auto player = entityManager->GetClosestPlayerWithin(position, -1);
		if (this->EntityAlive() && player) {
			double distance = position.DistanceSquared(player->position);

			// > 128 blocks away, despawn
			if (distance > 16384.0) {
				isDead = true;
				return true;
			}

			// > 32 blocks away? Try to despawn with a 1/800 chance
			if (this->age > 600 && rand.NextInt(800) == 0) {
				if (distance < 1024.0) {
					// We are 32 blocks or closer to a player
					age = 0;
					return false;
				}
				isDead = true;
				return true;
			}
		}
		return false;
	}
};