/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include "entity.h"
#include "entity_player.h"

struct MinecartEntity : public Entity {
	int damageTaken = 0;
	int shakeTimer = 0;
	int forwardDirection = 1;

	MinecartEntity() : Entity() {
		type = EntityType::MINECART;
		preventEntitySpawning = true;
		actsAsWorldCollider = false;
		width = 0.98f;
		height = 0.7f;
		yOffset = height / 2.0f;
		stepHeight = 0.0f;
		RebuildCollider();
	}
	~MinecartEntity() = default;

	bool CanBePushed() override {
		return true;
	}

	std::optional<AABB> GetMoverCollisionOverride(Entity& _candidate) override {
		return _candidate.collider;
	}

	float GetMountOffset() override {
		return -0.3f;
	}

	Vec3 GetRiderSeatOffset() override {
		double yawRad = double(rotationYaw) * (JavaMath::PI / 180.0);
		return { std::cos(yawRad) * 0.4, 0.0, std::sin(yawRad) * 0.4 };
	}

	bool AttackEntityFrom(Entity* _entity, int _damage) override;
	void Tick() override;
	void OnPlayerInteract(PlayerEntity* _entity) override;

private:
	void DropAsItems();
};