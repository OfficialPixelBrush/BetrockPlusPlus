/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_item.h"
#include "world/world.h"
#include "entity_player.h"
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
	ticksExisted++;
	pickupCooldown--;
	pickupCooldown = std::max(int8_t(0), pickupCooldown);

	// Update what block is below us
	int bx = (int)std::floor(posX);
	int by = (int)std::floor(posY - 0.2 - yOffset);
	int bz = (int)std::floor(posZ);
	int blockId = world->getBlockId({ bx, by, bz });
	if (world->getBlockId({ bx, by - 1, bz }) == BLOCK_FENCE) {
		blockId = world->getBlockId({ bx, by - 1, bz });
	}
	belowBlock = Blocks::blockProperties[blockId];

	if (hasPhysics) {
		if (inWater) {
			moveInFluid(WATER_DRAG);
		} else if (inLava) {
			moveInFluid(LAVA_DRAG);
		} else {
			motionY -= 0.04;
			move({ motionX, motionY, motionZ });

			float horizontalDrag = 0.98f;
			if (onGround) {
				horizontalDrag = 0.588f;
				if (blockId > 0)
					horizontalDrag = belowBlock.slipperiness * 0.98f;
			}

			motionX *= double(horizontalDrag);
			motionY *= 0.98;
			motionZ *= double(horizontalDrag);

			// Bounce when we land
			if (onGround)
				motionY *= -0.5;
		}
	}
}