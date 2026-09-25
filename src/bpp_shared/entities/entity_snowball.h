/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_throwable.h"
#include "logger.h"

struct SnowballEntity : public ThrowableEntity {
	// Default
	SnowballEntity() : ThrowableEntity() {
		type = EntityType::THROWN_SNOWBALL;
	}

	// Spawn from position
	SnowballEntity(Vec3 _position) : ThrowableEntity(_position) {
		type = EntityType::THROWN_SNOWBALL;
	}

	// Spawn from entity
	SnowballEntity(std::shared_ptr<MobileEntity> _owner) : ThrowableEntity(_owner) {
		type = EntityType::THROWN_SNOWBALL;
	}
};