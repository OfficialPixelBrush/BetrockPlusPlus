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
public:
	Pathfinder pathFinder;
	std::vector<Int3> currentPath;
	size_t currentPathIdx = 0;

	int64_t age = 0;

	void ResolveEntityCollision(Entity& _other);
	void TickPhysics();
	void ResolveEntityPushes();

	MobileEntity();

	int health = 20;
	int lastHealth = this->GetHeartsHealth();
	int maxHealth = 20;
	int maxHurtTime = 20;
	int lastAttackDamage = 0;
	int deathTime = 0;
	int attackTime = 0;
	float accumulatedFractionalDamage = 0.0f;
	float movementSpeed = 0.7f;
	bool canBreatheUnderwater : 1 = false;
	bool beenAttacked : 1 = false;
	ItemStack heldItem;
	ItemStack* armor[4] = { nullptr, nullptr, nullptr, nullptr }; // Helmet, chestplate, leggings, boots

	virtual void Tick() override;
	virtual void OnDeath(Entity* _killer);
	virtual void SetGoal(std::optional<Int3> _goal);
	virtual bool onLadder();
	void Heal(int _health);
	bool AABBNotInLiquidOrObstructed(AABB& _collider);
	bool HeadInOpaqueBlock();
	bool HeadInWater();
	ItemStack* GetHeldItem();
	void SetHeldItem(ItemStack _stack);
	bool FollowPath();
	void DealDamage(int _damage);
	int GetArmorValue();
	bool AttackEntityFrom(Entity* _entity, int _damage) override;
	std::optional<Tag> SerializeToNbt() override;
	void LoadFromNbt(Tag& _nbt) override;
	float GetEyeHeight() {
		return height * 0.85f;
	}
	void SetMaxHealth(int _health) {
		this->health = _health;
		this->maxHealth = _health;
		this->lastHealth = _health;
	}
	bool CanBePushed() override {
		return true;
	}
	const int GetHeartsHealth() {
		return this->health;
	}
	bool EntityAlive() {
		return !isDead && health > 0;
	}
	virtual bool CanSpawnAt(Int3 _pos) {
		if (!world)
			return false;
		bool clearOfEntities = entityManager->GetEntitiesWithinAabb(this->collider).size() == 0;
		bool clearOfBlocks = world->GetCollidingBoundingBoxes(this->collider).size() == 0;
		bool clearOfLiquid = !world->IsLiquidInAabb(this->collider);
		return clearOfBlocks && clearOfEntities && clearOfLiquid;
	}
};