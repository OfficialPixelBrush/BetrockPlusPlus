/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_item.h"
#include "entity_player.h"
#include "world/world.h"
#include <algorithm>
#include <cmath>

void ItemEntity::onCollideWithPlayer(PlayerEntity& entity) {
	if (pickupCooldown)
		return;
	entity.pickupItem(this->itemStack, this->id);
	if (this->itemStack.count <= 0)
		this->isDead = true;
}

void ItemEntity::tick() {
	// Item entities have differing physics
	Entity::tick();
	pickupCooldown--;
	pickupCooldown = std::max(int8_t(0), pickupCooldown);

	motionY -= 0.04;

	if (world->getMaterial({ MathHelper::floor_double(posX), MathHelper::floor_double(posY),
	                         MathHelper::floor_double(posZ) }) == Material::Lava()) {
		motionY = 0.2;
		motionX = double((rand.nextFloat() - rand.nextFloat()) * 0.2F);
		motionZ = double((rand.nextFloat() - rand.nextFloat()) * 0.2F);
	}

	pushOutOfBlocks({ posX, (collider.minY + collider.maxY) / 2.0, posZ });
	move({ motionX, motionY, motionZ });

	float horizontalDrag = 0.98f;
	if (onGround) {
		horizontalDrag = 0.58800006f;

		// Look up the block below us
		int bx = MathHelper::floor_double(posX);
		int by = MathHelper::floor_double(collider.minY) - 1;
		int bz = MathHelper::floor_double(posZ);
		int blockId = world->getBlockId({ bx, by, bz });
		belowBlock = Blocks::blockProperties[blockId];

		if (blockId > 0)
			horizontalDrag = belowBlock.slipperiness * 0.98f;
	}

	motionX *= double(horizontalDrag);
	motionY *= 0.9800000190734863;
	motionZ *= double(horizontalDrag);

	// Bounce when we land
	if (onGround)
		motionY *= -0.5;

	if (ticksExisted >= 6000)
		isDead = true;
}