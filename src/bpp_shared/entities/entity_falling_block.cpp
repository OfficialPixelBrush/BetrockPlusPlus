/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entities/entity_item.h"
#include "entity_falling_block.h"
#include "java_math.h"
#include "world/world.h"

void FallingBlockEntity::tick() {
	if (block == BLOCK_AIR) {
		isDead = true;
		return;
	}

	ticksFallen++;
	velocity.y -= 0.04;
	move(this->velocity);
	velocity *= { 0.98, 0.98, 0.98 };

	auto fd = MathHelper::floor_double;
	Int3 blockPosition = { fd(position.x), fd(position.y), fd(position.z) };

	if (onGround) {
		velocity *= { 0.7, -0.5, 0.7 };
		isDead = true;
		// TODO: check if we can actually fall here properly
		this->world->setBlock(blockPosition, this->block, 0);
		return;
	}
	if (ticksFallen > 100) {
		// Create the item entity
		Vec3 itemPos = position;
		std::shared_ptr<ItemEntity> itemEntity = std::make_shared<ItemEntity>(itemPos);
		itemEntity->itemStack = {this->block, 1};
		itemEntity->dim = dim;

		// Register our item with the world
		this->world->entityManager.addEntity(std::move(itemEntity));
		isDead = true;
	}
}