/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_mobile.h"

enum SleepFailureReason : uint8_t {
	CANT_SLEEP_AT_POSITION,
	CANT_SLEEP_AT_TIME,
	TOO_FAR,
	ALREADY_SLEEPING,
	OTHER,
	SUCCESS
};

struct PlayerEntity : public MobileEntity {
	bool isSleeping = false;

	// Position of the bed (headboard) the player is currently sleeping in.
	// Only meaningful while isSleeping is true.
	Int3 bedPosition{ 0, 0, 0 };
	// Also called "SleepTimer" in the player save
	TickTime ticksInBed = 0;
	Vec3 spawnPos = {};
	PlayerEntity() : MobileEntity() {
		type = EntityType::PLAYER;
		width = 0.6f;
		height = 1.8f;
		stepHeight = 0.5f;
	}
	~PlayerEntity() = default;
	bool AttackEntityFrom(Entity* _entity, int _damage) override;
	virtual void Tick() override;
	virtual bool PickupItem(ItemStack& _stack, EntityId _entityId);
	virtual bool DropItem(ItemStack _stack);
	virtual void DropInventory();
	virtual void OnDeath(Entity* _killer) override;
	virtual void OnMountEntity() override;
	virtual void OnDismountEntity() override;
	virtual SleepFailureReason TrySleep(Int3 _pos);
	virtual void WakeUp(bool _confirmSpawn = true);
};