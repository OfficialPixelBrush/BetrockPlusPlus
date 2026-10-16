/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */

#include "entity_mobile.h"

void MobileEntity::SetGoal(std::optional<Int3> _goal) {
	if (!_goal) {
		currentPath.clear();
		currentPathIdx = 0;
		return;
	}
	auto fd = MathHelper::FloorDouble;
	Int3 start = { fd(collider.minX), fd(collider.minY), fd(collider.minZ) };
	currentPath = pathFinder.FindPath(start, *_goal, width, height);
	currentPathIdx = 0;
}

bool MobileEntity::FollowPath() {
	jumping = false;

	// Mobs bob/hop while in water or lava
	if ((inWater || inLava) && rand.NextFloat() < 0.8f) {
		jumping = true;
	}

	if (currentPath.empty()) {
		return false;
	}

	// 1% chance to abandon our path
	if (rand.NextInt(100) == 0) {
		currentPath.clear();
		currentPathIdx = 0;
		return false;
	}

	int footprintSpan = int(width + 1.0f);
	double halfSpan = footprintSpan * 0.5;

	bool justFinished = false;

	while (currentPathIdx < currentPath.size()) {
		Int3 p = currentPath[currentPathIdx];
		double dx = p.x + halfSpan - position.x;
		double dz = p.z + halfSpan - position.z;

		double threshold = width * 2.0;
		if (dx * dx + dz * dz > threshold * threshold)
			break;

		currentPathIdx++;
		if (currentPathIdx >= currentPath.size())
			justFinished = true;
	}

	if (currentPathIdx >= currentPath.size()) {
		currentPath.clear();
		currentPathIdx = 0;

		if (justFinished && collidedHorizontally)
			jumping = true;
	} else {
		Int3 target = currentPath[currentPathIdx];
		double dx = target.x + halfSpan - position.x;
		double dz = target.z + halfSpan - position.z;

		float targetYaw = std::atan2(dz, dx) * 180.0 / JavaMath::PI - 90.0;
		float yawDelta = targetYaw - rotationYaw;

		while (yawDelta < -180.0F)
			yawDelta += 360.0F;
		while (yawDelta >= 180.0F)
			yawDelta -= 360.0F;
		yawDelta = std::clamp(yawDelta, -30.0F, 30.0F);

		rotationYaw += yawDelta;

		input.y = movementSpeed;

		int floorY = MathHelper::FloorDouble(collider.minY + 0.5);
		double dy = target.y - floorY;

		if (dy > 0.0)
			jumping = true;
	}

	return true;
}

void MobileEntity::Tick() {
	if (pathFinder.world == nullptr) {
		pathFinder.world = world;
	}

    LivingEntity::Tick();
}