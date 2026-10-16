/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "camera.h"
#include <algorithm>
#include <cmath>
#include <glm/ext/vector_float3.hpp>

void Camera::UpdateLook(Float2 _delta) {
	yaw += _delta.x;
	pitch -= _delta.y;

	pitch = std::clamp(pitch, -89.0f, 89.0f);
}

void Camera::Move(glm::vec3 _input, float _deltaTime) {
	constexpr float BASE_SPEED = 2.5f;

	float yawRad = glm::radians(yaw);

	glm::vec3 forward = { std::sin(yawRad), 0.0f, -std::cos(yawRad) };

	glm::vec3 right = { std::cos(yawRad), 0.0f, std::sin(yawRad) };

	glm::vec3 movement = glm::vec3(0.0f, _input.y, 0.0f) + forward * _input.z + right * _input.x;

	float length = glm::length(movement);
	if (length > 1.0f)
		movement /= length;

	position += movement * BASE_SPEED * _deltaTime;
}

glm::mat4 Camera::GetViewProj(float _aspect) {
	glm::mat4 proj = glm::perspective(glm::radians(fov), _aspect, 0.05f, 2000.0f);

	glm::vec3 direction = { std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch)), std::sin(glm::radians(pitch)),
		                    -std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)) };

	glm::mat4 view = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));

	return proj * view;
}