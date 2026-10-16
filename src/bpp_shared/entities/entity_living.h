/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include "entity.h"
#include "inventory/item_stack.h"
#include "numeric_structs.h"
#include <optional>

struct LivingEntity : public Entity {
public:
	int64_t age = 0;

	LivingEntity();

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

	// Footstep stuff
	float distanceWalkedModified = 0.0f;
	int nextStepDistance = 1;
	bool canTriggerWalking = true;

	virtual void ResolveEntityCollision(Entity& _other);
	virtual void TickPhysics();
	virtual void ResolveEntityPushes();
	virtual void Tick() override;
	virtual void OnDeath(Entity* _killer);
	virtual bool IsOnLadder();
	virtual void UpdateAIState() {}
	virtual void Move(Vec3& _velocity) override;
	void Heal(int _health);
	bool AABBNotInLiquidOrObstructed(AABB& _collider);
	bool HeadInOpaqueBlock();
	bool HeadInWater();
	ItemStack* GetHeldItem();
	void SetHeldItem(ItemStack _stack);
	void DealDamage(int _damage);
	int GetArmorValue();
	virtual bool AttackEntityFrom(Entity* _entity, int _damage) override;
	std::optional<Tag> SerializeToNbt() override;
	void LoadFromNbt(Tag& _nbt) override;
	virtual bool IsMovementBlocked() {
		return health <= 0;
	}
	virtual float GetEyeHeight() {
		return height * 0.85f;
	}
	bool CanBeCollidedWith() override {
		return !isDead;
	}
	void SetMaxHealth(int _health) {
		this->health = _health;
		this->maxHealth = _health;
		this->lastHealth = _health;
	}
	bool CanBePushed() override {
		return true;
	}
	int GetHeartsHealth() const {
		return this->health;
	}
	bool EntityAlive() {
		return !isDead && health > 0;
	}
};