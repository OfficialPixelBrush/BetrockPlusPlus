/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_skeleton.h"

void SkeletonEntity::OnDeath() {
	// Drop arrows
	auto targetItem = Items::Id::ARROW;
	int itemCount = this->rand.NextInt(3);

	for (int i = 0; i < itemCount; i++) {
		DropItemAtEntity(targetItem, 1);
	}

	// Drop bones
	targetItem = Items::Id::BONE;
	itemCount = this->rand.NextInt(3);

	for (int i = 0; i < itemCount; i++) {
		DropItemAtEntity(targetItem, 1);
	}
}