/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "camera.h"
#include <algorithm>

void Camera::Move(glm::vec2 _input, float _deltaTime) {
	constexpr float BASE_SPEED = 2.5f;
	float speed = BASE_SPEED * _deltaTime;

	glm::vec3 right = glm::normalize(glm::cross(forward, up));

	position += forward * (_input.y * speed);

	position += right * (_input.x * speed);
}

void Camera::UpdateLook(Float2 _delta) {
	yaw += _delta.x;
	pitch += _delta.y;

	// Prevent flipping over
	pitch = std::clamp(pitch, -89.0f, 89.0f);

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = -sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	forward = glm::normalize(direction);
}

glm::mat4 Camera::GetViewProj(float _aspect) {
	glm::mat4 proj = glm::perspective(glm::radians(70.0f), _aspect, 0.05f, 2000.0f);

	glm::mat4 view = glm::lookAt(position, position + forward, up);

	return proj * view;
}