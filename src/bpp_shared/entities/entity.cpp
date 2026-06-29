/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
 */
#include "entity.h"
#include "world/world.h"
#include <algorithm>
#include <cmath>

void Entity::tick() {
	ticksExisted++;

	// Input axes are decayed every tick before new input is added
	moveForward *= INPUT_DECAY;
	moveStrafe *= INPUT_DECAY;

	// Update what block is below us
	int bx = (int)std::floor(posX);
	int by = (int)std::floor(posY - 0.2 - yOffset);
	int bz = (int)std::floor(posZ);
	int blockId = world->getBlockId({ bx, by, bz });
	if (world->getBlockId({ bx, by - 1, bz }) == BLOCK_FENCE) {
		blockId = world->getBlockId({ bx, by - 1, bz });
	}
	belowBlock = Blocks::blockProperties[blockId];

	if (sneaking) {
		moveForward *= SNEAK_SPEED_MODIFIER;
		moveStrafe *= SNEAK_SPEED_MODIFIER;
	}

	if (jumping) {
		if (inWater || inLava) {
			motionY += FLUID_JUMP_BOOST;
		} else if (onGround) {
			motionY = JUMP_VELOCITY;
		}
	}

	if (inWater) {
		moveInFluid(WATER_DRAG);
	} else if (inLava) {
		moveInFluid(LAVA_DRAG);
	} else {
		// Friction depends on the block underfoot; slippery blocks reduce both braking and acceleration
		float friction = onGround ? belowBlock.slipperiness * HORIZONTAL_FRICTION : HORIZONTAL_FRICTION;

		// Acceleration is tuned so that on normal ground (slipperiness 0.6) it equals exactly 0.1
		float acceleration = onGround ? 0.1f * (NORMAL_FRICTION_CUBED / (friction * friction * friction))
		                              : AIR_ACCELERATION;

		applyInput(moveStrafe, moveForward, acceleration);

		if (onLadder) {
			motionX = std::clamp(float(motionX), -LADDER_MAX_HORIZONTAL, LADDER_MAX_HORIZONTAL);
			motionY = std::max(float(motionY), sneaking ? LADDER_SNEAK_DESCENT : -LADDER_MAX_DESCENT);
			motionZ = std::clamp(float(motionZ), -LADDER_MAX_HORIZONTAL, LADDER_MAX_HORIZONTAL);
			fallDistance = 0;
		}

		move({ motionX, motionY, motionZ });

		// Climb upward when pressing into a ladder
		if (collidedHorizontally && onLadder) {
			motionY = LADDER_WALL_BOOST;
		}

		motionY -= GRAVITY;
		motionX *= friction;
		motionY *= VERTICAL_FRICTION;
		motionZ *= friction;
	}
}

void Entity::moveInFluid(float drag) {}

void Entity::applyKnockback(Vec3 direction) {
	motionX *= KNOCKBACK_VELOCITY_DAMPENING;
	motionY *= KNOCKBACK_VELOCITY_DAMPENING;
	motionZ *= KNOCKBACK_VELOCITY_DAMPENING;
	motionX -= direction.x * HORIZONTAL_KNOCKBACK;
	motionZ -= direction.z * HORIZONTAL_KNOCKBACK;
	motionY = std::min(float(motionY + VERTICAL_KNOCKBACK), VERTICAL_KNOCKBACK);
}

void Entity::applyInput(float strafe, float forward, float acceleration) {
	float length = std::sqrt((strafe * strafe) + (forward * forward));

	if (length < 0.01f)
		return;

	if (length < 1.0f)
		length = 1.0f;

	strafe /= length;
	forward /= length;

	motionX += (strafe * std::cos(rotationYaw) - forward * std::sin(rotationYaw)) * acceleration;
	motionZ += (forward * std::cos(rotationYaw) + strafe * std::sin(rotationYaw)) * acceleration;
}

void Entity::move(Vec3 movement) {
	ySize *= 0.4f;

	if (inWeb) {
		inWeb = false;
		movement.x *= COBWEB_HORIZONTAL_DRAG;
		movement.y *= COBWEB_VERTICAL_DRAG;
		movement.z *= COBWEB_HORIZONTAL_DRAG;
		motionX = 0.0;
		motionY = 0.0;
		motionZ = 0.0;
	}

	Vec3 original = movement;

	if (onGround && sneaking) {
		sneakClipMovement(movement);
	}

	resolveCollisions(movement, original);
	updateFallState(movement.y);
}

void Entity::sneakClipMovement(Vec3& movement) {}

void Entity::resolveCollisions(Vec3& movement, const Vec3& original) {}

void Entity::dealDamage(int amount) {}

void Entity::updateFallState(float movedY) {
	if (onGround) {
		if (fallDistance > FALL_DAMAGE_FLOOR) {
			dealDamage((int)std::ceil(fallDistance - FALL_DAMAGE_FLOOR));
		}
		fallDistance = 0;
	} else if (movedY < 0) {
		fallDistance -= movedY;
	}
}