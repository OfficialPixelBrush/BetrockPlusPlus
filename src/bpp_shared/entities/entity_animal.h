/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_mob.h"

struct AnimalEntity : public MobEntity {
	AnimalEntity() : MobEntity() {}
	~AnimalEntity() = default;

	void OnDeath() override;
	void Wander() override;
	float GetWanderWeight(Int3 _pos) override;
	void Tick() override {
		MobileEntity::Tick();
		
		if (EntityAlive()) {
			Wander();
			FollowPath();
		}

		TryDespawn();
	}
	bool CanSpawnAt(Int3 _pos) override {
		if (!world)
			return false;
		auto fd = MathHelper::FloorDouble;
		Int3 footPos = { fd(position.x), fd(collider.minY), fd(position.z) };
		return world->GetBlockId({ footPos.x, footPos.y - 1, footPos.z }) == BLOCK_GRASS &&
		       world->getBlockLightFull(footPos) > 8 && MobileEntity::CanSpawnAt(_pos); 
	}
};