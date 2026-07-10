/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity.h"
#include "entity_player.h"
#include "world/world.h"
#include "entity_manager.h"
#include <algorithm>
#include <cmath>

void Entity::onCollideWithPlayer(PlayerEntity& entity) {
	return;
}

void Entity::tick() {
	ticksExisted++;
}

void Entity::moveInFluid([[maybe_unused]] float drag) {}

void Entity::applyKnockback(Vec3 direction) {
	motionX *= KNOCKBACK_VELOCITY_DAMPENING;
	motionY *= KNOCKBACK_VELOCITY_DAMPENING;
	motionZ *= KNOCKBACK_VELOCITY_DAMPENING;
	motionX -= direction.x * HORIZONTAL_KNOCKBACK;
	motionZ -= direction.z * HORIZONTAL_KNOCKBACK;
	motionY = std::min(float(motionY + VERTICAL_KNOCKBACK), VERTICAL_KNOCKBACK);
	velocityChanged = true;
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
	collider = collider.offset(0.0, movement.y, 0.0);

	// Check if we are on ground or landed this tick
	bool canStepUp = onGround || (original.y != movement.y && original.y < 0.0);

	// Resolve X
	for (auto& col : sweptCollider) {
		movement.x = col.calculateXOffset(collider, movement.x);
	}
	collider = collider.offset(movement.x, 0.0, 0.0);

	// Resolve Z
	for (auto& col : sweptCollider) {
		movement.z = col.calculateZOffset(collider, movement.z);
	}
	collider = collider.offset(0.0, 0.0, movement.z);

	collidedHorizontally = original.x != movement.x || original.z != movement.z;

	if (stepHeight > 0.0f && canStepUp && (clampSneak || ySize < 0.05f) && collidedHorizontally) {
		auto stepUpMovement = movement;
		movement = { original.x, stepHeight, original.z };

		AABB resolvedCollider = collider;
		collider = originalCollider;

		auto stepUpSweptCollider = world->getCollidingBoundingBoxes(
		    collider.addCoord(movement.x, movement.y, movement.z));

		// Resolve Y first
		for (auto& col : stepUpSweptCollider) {
			movement.y = col.calculateYOffset(collider, movement.y);
		}
		collider = collider.offset(0.0, movement.y, 0.0);

		// Resolve X
		for (auto& col : stepUpSweptCollider) {
			movement.x = col.calculateXOffset(collider, movement.x);
		}
		collider = collider.offset(movement.x, 0.0, 0.0);

		// Resolve Z
		for (auto& col : stepUpSweptCollider) {
			movement.z = col.calculateZOffset(collider, movement.z);
		}
		collider = collider.offset(0.0, 0.0, movement.z);

		// Snap down
		double downY = -stepHeight;
		for (auto& col : stepUpSweptCollider) {
			downY = col.calculateYOffset(collider, downY);
		}
		collider = collider.offset(0.0, downY, 0.0);

		// Keep whichever collision path moved further horizontally
		if (stepUpMovement.x * stepUpMovement.x + stepUpMovement.z * stepUpMovement.z >
		    movement.x * movement.x + movement.z * movement.z) {
			movement = stepUpMovement;
			collider = resolvedCollider;
		} else {
			movement.y += (resolvedCollider.minY - originalCollider.minY) - stepHeight;
			double frac = resolvedCollider.minY - collider.minY;
			if (frac > 0.0)
				ySize += float(frac + 0.01);
		}
	}

	// Derive our current position from our collider
	posX = (collider.minX + collider.maxX) / 2.0;
	posY = collider.minY + double(yOffset) - double(ySize);
	posZ = (collider.minZ + collider.maxZ) / 2.0;

	collidedHorizontally = original.x != movement.x || original.z != movement.z;
	collidedVertically = original.y != movement.y;
	onGround = original.y != movement.y && original.y < 0.0;
	collided = collidedHorizontally || collidedVertically;

	if (original.x != movement.x)
		motionX = 0.0;
	if (original.y != movement.y)
		motionY = 0.0;
	if (original.z != movement.z)
		motionZ = 0.0;

	updateFallState(movement.y);
}

void Entity::dealDamage([[maybe_unused]] int amount) {}

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