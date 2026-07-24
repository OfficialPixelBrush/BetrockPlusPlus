/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_falling_block.h"
#include "entities/entity_item.h"
#include "java_math.h"
#include "world/world.h"

void FallingBlockEntity::Tick() {
	if (block == BLOCK_AIR) {
		isDead = true;
		return;
	}

	ticksFallen++;
	velocity.y -= 0.04;
	Move(this->velocity);
	velocity *= { 0.98, 0.98, 0.98 };

	auto fd = MathHelper::FloorDouble;
	Int3 blockPosition = { fd(position.x), fd(position.y), fd(position.z) };

	if (onGround) {
		velocity *= { 0.7, -0.5, 0.7 };
		isDead = true;
		if (!Blocks::CanFallAt(*this->world, blockPosition)) {
			this->DropItemAtEntity(this->block, 1);
			return;
		}
		this->world->SetBlock(blockPosition, this->block, 0);
		return;
	}
	if (ticksFallen > 100) {
		// Create the item entity
		this->DropItemAtEntity(this->block, 1);
		isDead = true;
	}
}