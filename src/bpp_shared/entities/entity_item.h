/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include "entity.h"

struct ItemEntity : public Entity {
	ItemEntity(Vec3 position) : Entity() {
		type = "Item";
		width = 0.25f;
		height = 0.25f;
		yOffset = 0.125f; // height / 2

		// Set the initial position of the item entity
		this->teleport(position);

		// This stuff is mostly randomized
		rotationYaw = Java::Random().nextDouble() * 360.0;
		motionX = Java::Random().nextDouble() * 0.2 - 0.1;
		motionY = 0.2;
		motionZ = Java::Random().nextDouble() * 0.2 - 0.1;
	}
};