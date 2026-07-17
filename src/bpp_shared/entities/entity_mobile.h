/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include "entity.h"
#include "numeric_structs.h"
#include "pathfinding/pathfinder.hpp"
#include "world.h"
#include <optional>
#include <vector>

//TODO: Refactor specific parts into LivingEntity, and other classes
struct MobileEntity : public Entity {
private:
	Pathfinder pathFinder;
	std::vector<Int3> currentPath;
	size_t currentPathIdx = 0;

	void followPath();
	void resolveEntityCollision(Entity& other);
	void tickPhysics();

public:
	MobileEntity(WorldManager& world);

	void setGoal(std::optional<Int3> goal);
	void tick() override;
};