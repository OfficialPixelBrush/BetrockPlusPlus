/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_chicken.h"

void ChickenEntity::OnDeath(Entity* _killer) {
	// Drop 0-2 feathers
	auto targetItem = Items::Id::FEATHER;
	int itemCount = this->rand.NextInt(3);

	for (int i = 0; i < itemCount; i++) {
		DropItemAtEntity(targetItem, 1);
	}
}

void ChickenEntity::UpdateFallState(float _movedY) {
	fallDistance = 0;
}
