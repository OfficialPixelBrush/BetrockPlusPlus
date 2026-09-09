/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_pig.h"
#include "entity_player.h"

void PigEntity::OnDeath(Entity* _killer) {
	// Drop 0-2 porkchops, cooked if the pig was on fire
	auto targetItem = this->fireTicks > 0 ? Items::Id::PORKCHOP_COOKED : Items::Id::PORKCHOP;
	int itemCount = this->rand.NextInt(3);

	for (int i = 0; i < itemCount; i++) {
		DropItemAtEntity(targetItem, 1);
	}

	if (saddled)
		DropItemAtEntity(Items::SADDLE, 1);
}

void PigEntity::OnPlayerInteract(PlayerEntity* _entity) {
	if (!_entity || !saddled)
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
