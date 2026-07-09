/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "entities.h"
#include "entities/entity.h"

struct PlayerSession;
struct EntityMPPlayer : public Entity {
	PlayerSession* session = nullptr;
	EntityMPPlayer() : Entity() {
		type = EntityType::PLAYER;
		hasPhysics = false;
		width = 0.6f;
		height = 1.8f;
		yOffset = 1.62f;
	}
	~EntityMPPlayer() {
		session = nullptr;
	}
	void tick() override;
};