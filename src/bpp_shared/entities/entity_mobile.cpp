/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */

#include "entity_mobile.h"
#include "entities/entity.h"
#include "java_math.h"
#include <algorithm>
#include <cmath>
#include <optional>

MobileEntity::MobileEntity(WorldManager& world) : pathFinder(world) {
	type = EntityType::CREEPER;
}

void MobileEntity::setGoal(std::optional<Int3> goal) {
	if (!goal) {
		currentPath.clear();
		currentPathIdx = 0;
		return;
	}
	Int3 start = { MathHelper::floor_double(position.x), MathHelper::floor_double(position.y),
		           MathHelper::floor_double(position.z) };
	currentPath = pathFinder.findPath(start, *goal);
	currentPathIdx = 0;
	std::cout << "Calculated path!" << std::endl;
	for (auto& node : currentPath) {
		std::cout << node << std::endl;
	}
}

void MobileEntity::followPath() {
	moveForward = 0.0f;
	jumping = false;

	if (currentPath.empty())
		return;

	while (currentPathIdx < currentPath.size()) {
		Int3 p = currentPath[currentPathIdx];
		double dx = p.x + 0.5 - position.x;
		double dz = p.z + 0.5 - position.z;

		double threshold = width * 2.0;
		if (dx * dx + dz * dz > threshold * threshold)
			break;

		currentPathIdx++;
	}

	// Finished path
	if (currentPathIdx >= currentPath.size()) {
		currentPath.clear();
		currentPathIdx = 0;
		return;
	}

	Int3 target = currentPath[currentPathIdx];
	double dx = target.x + 0.5 - position.x;
	double dz = target.z + 0.5 - position.z;

	float targetYaw = std::atan2(dz, dx) * 180.0 / JavaMath::PI - 90.0;
	float yawDelta = targetYaw - rotationYaw;

	while (yawDelta < -180.0F)
		yawDelta += 360.0F;
	while (yawDelta >= 180.0F)
		yawDelta -= 360.0F;
	yawDelta = std::clamp(yawDelta, -30.0F, 30.0F);

	rotationYaw += yawDelta;

	moveForward = 0.7f;

	int floorY = MathHelper::floor_double(collider.minY + 0.5);
	double dy = target.y - floorY;

	if (dy > 0.0)
		jumping = true;
}

void MobileEntity::resolveEntityCollision(Entity& other) {
	if (rider == &other || passenger == &other)
		return;

	Vec2 delta = { other.position.x - position.x, other.position.z - position.z };
	double dist = MathHelper::abs_max(delta.x, delta.y);

	if (dist >= 0.01) {
		dist = std::sqrt(dist);

		delta = delta / dist;

		double forceScale = 1.0 / dist;

		if (forceScale > 1.0) {
			forceScale = 1.0;
		}

		delta = delta * forceScale;
		delta = delta * 0.05;

		this->motionX -= delta.x;
		this->motionZ -= delta.y;
		other.motionX += delta.x;
		other.motionZ += delta.y;
	}
}

void MobileEntity::tickPhysics() {
	//TODO: Handle water/lava/ladders and possibly other stuff

	// Normal ground/air movement
	float friction = 0.91f;
	if (onGround) {
		// Default block slipperiness is 0.6. (0.6 * 0.91 = 0.546)
		friction = 0.546f;
	}

	if (onGround) {
		float acceleration = 0.16277136f / (friction * friction * friction);
		applyInput(moveStrafe, moveForward, 0.1f * acceleration);
		if (jumping)
			motionY = 0.42;
	} else {
		// In air
		applyInput(moveStrafe, moveForward, 0.02f);
	}

	// Move the entity
	move({ motionX, motionY, motionZ });

	// Apply friction
	motionX *= friction;
	motionZ *= friction;

	// Gravity
	motionY -= 0.08;
	motionY *= 0.98f;

	auto collidingEntities = world->entityManager.getEntitiesWithinAABBExcluding(collider.expand(0.2, 0.0, 0.2), id);

	for (const auto& entity : collidingEntities) {
		//TODO: Only minecarts, boats and (not dead) living entities can be pushed
		//if (entity.canBePushed()) {
		this->resolveEntityCollision(*entity);
		//}
	}
}

void MobileEntity::tick() {
	Entity::tick();
	followPath();
	tickPhysics();
}