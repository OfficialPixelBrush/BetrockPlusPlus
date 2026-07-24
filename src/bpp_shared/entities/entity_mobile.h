/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include "entity.h"
#include "inventory/item_stack.h"
#include "numeric_structs.h"
#include "pathfinding/pathfinder.hpp"
#include <optional>
#include <vector>

//TODO: Refactor specific parts into LivingEntity, and other classes
struct MobileEntity : public Entity {
private:
	Pathfinder pathFinder;
	std::vector<Int3> currentPath;
	size_t currentPathIdx = 0;

	int64_t age = 0;

	void FollowPath();
	void ResolveEntityCollision(Entity& _other);
	void TickPhysics();

public:
	MobileEntity();

	int health = 20;
	int maxHealth = 20;
	int maxHurtTime = 20;
	int lastAttackDamage = 0;
	//float attackedAtYaw = 0.0f; // Already exist in base entity
	int deathTime = 0;
	int attackTime = 0;
	float eyeHeight = height * 0.85f;
	bool canBreatheUnderwater = false;
	ItemStack heldItem;

	const int GetHeartsHealth() {
		return this->health;
	}
	virtual void OnDeath();
	virtual void Tick() override;
	virtual void SetGoal(std::optional<Int3> _goal);
	bool AttackEntityFrom(Entity* _entity, int _damage) override;
	bool AABBNotInLiquidOrObstructed(AABB& _collider);
	bool HeadInOpaqueBlock();
	bool HeadInWater();
	bool EntityAlive() {
		return !isDead && health > 0;
	}
	ItemStack* GetHeldItem();
	void SetHeldItem(ItemStack _stack);
	bool onLadder();
	bool CanBePushed() override {
		return true;
	}
};