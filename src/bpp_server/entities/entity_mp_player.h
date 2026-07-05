/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "entities/entity.h"

struct PlayerSession;
struct EntityMPPlayer : public Entity {
	PlayerSession* session = nullptr;
	EntityMPPlayer() : Entity() {
		type = "Player";
		hasPhysics = false;
		width = 0.6f;
		height = 1.8f;
		yOffset = 1.62f;
	}
	void tick() override;
};