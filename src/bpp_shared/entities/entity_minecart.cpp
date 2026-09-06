/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_minecart.h"
#include "entity_manager.h"
#include "helpers/java/java_math.h"
#include "world/world.h"
#include <algorithm>
#include <cmath>

void MinecartEntity::DropAsItems() {
	DropItemAtEntity(Items::MINECART, 1);
	isDead = true;
}

bool MinecartEntity::AttackEntityFrom(Entity* _entity, int _damage) {
	if (isDead)
		return true;

	Entity::AttackEntityFrom(_entity, _damage);

	forwardDirection = -forwardDirection;
	shakeTimer = 10;
	damageTaken += _damage * 10;

	if (damageTaken > 40) {
		if (auto rider = passenger.lock())
			rider->UnmountEntity();
		DropAsItems();
	}

	return true;
}

void MinecartEntity::OnPlayerInteract(PlayerEntity* _entity) {
	if (!_entity)
		return;

	auto rider = passenger.lock();
	if (rider && rider.get() != _entity) {
		// Someone else is already riding
		return;
	}

	if (_entity == rider.get()) {
		_entity->UnmountEntity();
		return;
	}

	auto selfPtr = entityManager->GetEntityByIdShared(this->id);
	if (selfPtr)
		_entity->MountEntity(selfPtr);
}

void MinecartEntity::Tick() {
	Entity::Tick();
	if (shakeTimer > 0)
		shakeTimer--;
	if (damageTaken > 0)
		damageTaken--;
}