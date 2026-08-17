/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_pig.h"

void PigEntity::OnDeath(Entity* _killer) {
	// Drop 0-2 porkchops, cooked if the pig was on fire
	auto targetItem = this->fireTicks > 0 ? Items::Id::PORKCHOP_COOKED : Items::Id::PORKCHOP;
	int itemCount = this->rand.NextInt(3);

	for (int i = 0; i < itemCount; i++) {
		DropItemAtEntity(targetItem, 1);
	}
}
