/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_player.h"

bool PlayerEntity::pickupItem(ItemStack& _stack, EntityId _entityId) {
	return true;
}

bool PlayerEntity::dropItem(ItemStack _stack) {
	return true;
}