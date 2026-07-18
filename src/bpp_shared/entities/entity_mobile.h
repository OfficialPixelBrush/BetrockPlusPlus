/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include "entity.h"
#include "numeric_structs.h"
#include "pathfinding/pathfinder.hpp"
#include "world.h"
#include <optional>
#include <vector>

//TODO: Refactor specific parts into LivingEntity, and other classes
struct MobileEntity : public Entity {
private:
	Pathfinder pathFinder;
	std::vector<Int3> currentPath;
	size_t currentPathIdx = 0;

	int64_t age = 0;
	int health = 10;
	int maxHurtTime = 10;
	int lastAttackDamage = 0;
	float attackedAtYaw = 0.0f;
	int deathTime = 0;
	int attackTime = 0;
	float eyeHeight = height * 0.85f;
	bool canBreatheUnderwater = false;

	void followPath();
	void resolveEntityCollision(Entity& other);
	void tickPhysics();

public:
	MobileEntity(WorldManager& world);

	virtual void onDeath();
	virtual void tick() override;
	virtual void setGoal(std::optional<Int3> goal);
	bool attackEntityFrom(Entity* entity, int damage) override;
	bool AABBNotInLiquidOrObstructed(AABB& collider);
	bool headInOpaqueBlock();
	bool headInWater();
	bool entityAlive() {
		return !isDead && health > 0;
	}
	bool onLadder() {
		auto fd = MathHelper::floor_double;
		return world->getBlockId({ fd(position.x), fd(collider.minY), fd(position.z) }) == BLOCK_LADDER;
	}
	bool canBePushed() override {
		return true;
	}
};