/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_animal.h"

bool AnimalEntity::CanSpawnAt(Int3 _pos) {
	if (!world)
		return false;
	auto fd = MathHelper::FloorDouble;
	Int3 footPos = { fd(position.x), fd(collider.minY), fd(position.z) };
	return world->GetBlockId({ footPos.x, footPos.y - 1, footPos.z }) == BLOCK_GRASS &&
	       world->getBlockLightFull(footPos) > 8 && MobileEntity::CanSpawnAt(_pos);
}