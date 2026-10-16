/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include "entity_living.h"
#include "pathfinding/pathfinder.hpp"
#include <vector>

struct MobileEntity : public LivingEntity {
public:
	Pathfinder pathFinder;
	std::vector<Int3> currentPath;
	size_t currentPathIdx = 0;

	bool FollowPath();
	virtual void SetGoal(std::optional<Int3> _goal);

	virtual bool CanSpawnAt() {
		if (!world)
			return false;
		bool clearOfEntities = entityManager->GetEntitiesWithinAabb(this->collider).size() == 0;
		bool clearOfBlocks = world->GetCollidingBoundingBoxes(this->collider).size() == 0;
		bool clearOfLiquid = !world->IsLiquidInAabb(this->collider);
		return clearOfBlocks && clearOfEntities && clearOfLiquid;
	}

	virtual void Tick() override;
};