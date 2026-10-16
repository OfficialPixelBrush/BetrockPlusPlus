/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "entity_sp_player.h"

void EntitySPPlayer::UpdateAIState() {
	float yawRad = glm::radians(camera.yaw);

	Float2 forward = { std::sin(yawRad), -std::cos(yawRad) };

	Float2 right = { std::cos(yawRad), std::sin(yawRad) };

	float forwardFactor = playerInput.forward - playerInput.backward;
	float rightFactor = playerInput.right - playerInput.left;

	input = forward * forwardFactor + right * rightFactor;
}