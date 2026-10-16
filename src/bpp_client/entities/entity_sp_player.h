/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "../camera.h"
#include "entities/entity_player.h"

struct PlayerInput {
	bool forward : 1 = false;
	bool backward : 1 = false;
	bool left : 1 = false;
	bool right : 1 = false;
	bool jump : 1 = false;
	bool sprint : 1 = false;
	bool sneak : 1 = false;
};

struct EntitySPPlayer : public PlayerEntity {
public:
	PlayerInput playerInput{};

	EntitySPPlayer() : PlayerEntity() {}
	~EntitySPPlayer() = default;

	// AI is the human who controls this entity
	virtual void UpdateAIState() override;

private:
	Camera camera{};
};