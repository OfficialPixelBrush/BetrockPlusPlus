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
	AABB originalCollider = collider;
	bool clampSneak = onGround && sneaking;

	if (clampSneak) {
		const double step = 0.05;

		auto groundBelow = [&](double dx, double dz) -> bool {
			return !world->getCollidingBoundingBoxes(collider.offset(dx, -1.0, dz)).empty();
		};

		// Clamp on the X and Z axes to avoid falling off edges while sneaking
		while (movement.x != 0.0 && !groundBelow(movement.x, 0.0)) {
			if (movement.x < step && movement.x >= -step)
				movement.x = 0.0;
			else if (movement.x > 0.0)
				movement.x -= step;
			else
				movement.x += step;
		}
		while (movement.z != 0.0 && !groundBelow(0.0, movement.z)) {
			if (movement.z < step && movement.z >= -step)
				movement.z = 0.0;
			else if (movement.z > 0.0)
				movement.z -= step;
			else
				movement.z += step;
		}

		// Update our og values so step up logic uses the correct position
		original.x = movement.x;
		original.z = movement.z;
	}

	auto sweptCollider = world->getCollidingBoundingBoxes(collider.addCoord(movement.x, movement.y, movement.z));

	// Resolve Y first
	for (auto& col : sweptCollider) {
		movement.y = col.calculateYOffset(collider, movement.y);
	}
	collider.offset(0.0, movement.y, 0.0);

	// Check if we are on ground or landed this tick
	bool canStepUp = onGround || (original.y != movement.y && original.y < 0.0);

	// Resolve X
	for (auto& col : sweptCollider) {
		movement.x = col.calculateXOffset(collider, movement.x);
	}
	collider.offset(movement.x, 0.0, 0.0);

	// Resolve Z 
	for (auto& col : sweptCollider) {
		movement.z = col.calculateZOffset(collider, movement.z);
	}
	collider.offset(0.0, 0.0, movement.z);

	collidedHorizontally = original.x != movement.x || original.z != movement.z;

	if (stepHeight > 0.0f && canStepUp && (clampSneak || yOffset < 0.05f) && collidedHorizontally) {
		auto stepUpMovement = movement;
		movement = {original.x, stepHeight, original.z};
		
		AABB resolvedCollider = collider;
		collider = originalCollider;

		auto stepUpSweptCollider = world->getCollidingBoundingBoxes(
		    collider.addCoord(movement.x, movement.y, movement.z));

		// Resolve Y first
		for (auto& col : stepUpSweptCollider) {
			movement.y = col.calculateYOffset(collider, movement.y);
		}
		collider.offset(0.0, movement.y, 0.0);

		// Resolve X
		for (auto& col : stepUpSweptCollider) {
			movement.x = col.calculateXOffset(collider, movement.x);
		}
		collider.offset(movement.x, 0.0, 0.0);

		// Resolve Z
		for (auto& col : stepUpSweptCollider) {
			movement.z = col.calculateZOffset(collider, movement.z);
		}
		collider.offset(0.0, 0.0, movement.z);

		// Snap down
		double downY = -stepHeight; 
		for (auto& col : stepUpSweptCollider) {
			downY = col.calculateYOffset(collider, downY);
		}
		collider.offset(0.0, downY, 0.0);

		// Keep whichever collision path moved further horizontally
		if (stepUpMovement.x * stepUpMovement.x + stepUpMovement.z * stepUpMovement.z >
		    movement.x * movement.x + movement.z * movement.z) {
			movement = stepUpMovement;
			collider = resolvedCollider;
		}
		else {
			movement.y += (boundingBox.minY - originalCollider.minY) - stepHeight;
			double frac = boundingBox.minY - collider.minY;
			if (frac > 0.0)
				yOffset += float(frac + 0.01);
		}

	}

	// Derive our current position from our collider
	posX = (collider.minX + collider.maxX) / 2.0;
	posY = collider.minY;
	posZ = (collider.minZ + collider.maxZ) / 2.0;

	collidedHorizontally = original.x != movement.x || original.z != movement.z;
	collidedVertically = original.y != movement.y;
	onGround = original.y != movement.y && original.y < 0.0;
	isCollided = collidedHorizontally || collidedVertically;

	if (original.x != movement.x)
		motionX = 0.0;
	if (original.y != movement.y)
		motionY = 0.0;
	if (original.z != movement.z)
		motionZ = 0.0;

	updateFallState(movement.y);
}

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