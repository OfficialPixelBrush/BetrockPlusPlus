/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_mobile.h"

struct AnimalEntity : public MobileEntity {
	AnimalEntity() : MobileEntity() {
		width = 0.9f;
		height = 0.9f;
		stepHeight = 0.5f;
		SetMaxHealth(/*Health=*/10);
	}
	~AnimalEntity() = default;
	virtual void Wonder();
	virtual float GetWanderWeight(Int3 _pos);

	void OnDeath() override;
	void Tick() override {
		MobileEntity::Tick();
		
		if (EntityAlive()) {
			Wonder();
			FollowPath();
		}

		TryDespawn();
	}
	bool CanSpawnAt(Int3 _pos) {
		if (!world)
			return false;
		auto fd = MathHelper::FloorDouble;
		Int3 footPos = { fd(position.x), fd(collider.minY), fd(position.z) };
		return world->GetBlockId({ footPos.x, footPos.y - 1, footPos.z }) == BLOCK_GRASS &&
		       world->getBlockLightFull(footPos) > 8 && MobileEntity::CanSpawnAt(_pos); 
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