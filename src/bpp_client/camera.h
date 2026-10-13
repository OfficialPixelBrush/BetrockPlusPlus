/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "numeric_structs.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camera {
	float fov = 70.0f;
	glm::vec3 position;
	glm::vec3 forward = { 0.0f, 0.0f, -1.0f };
	glm::vec3 up = { 0.0f, 1.0f, 0.0f };
	float yaw = 0.0f, pitch = 0.0f;

	void Move(glm::vec2 _input, float _deltaTime);
	void UpdateLook(Float2 _delta);
	glm::mat4 GetViewProj(float _aspect);
};