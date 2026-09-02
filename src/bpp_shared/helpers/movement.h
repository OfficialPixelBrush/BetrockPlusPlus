/*
 * Copyright (c) 2026, Tiago F <tacf>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

// Movement maths to be shared between the server's entities and the client's
// prediction of its own player.
// Templates were used since server and client will use similar nonetheless
// distinct types. Could be improved in the future for sure.

#pragma once
#include "AABB.h"
#include <cmath>
#include <numeric_structs.h>
#include <span>
#include <vector>

namespace Movement {

// Beta's movement-input normalization: inputs below a threshold do nothing, and
// anything at or under unit length is left alone. Should help with analog sticks.
// Returns false when the input is too small to act on.
//
// It normalizes in place because Entity keeps the normalized value as state.
template <typename T>
bool NormalizeMoveInput(T& _strafe, T& _forward) {
	T length = std::sqrt(_strafe * _strafe + _forward * _forward);
	if (length < T(0.01))
		return false;
	if (length < T(1.0))
		length = T(1.0);
	_strafe /= length;
	_forward /= length;
	return true;
}

// Beta resolves a swept move one axis at a time, Y first, offsetting the
// collider after each axis so the next one tests against the updated box.
inline void ResolveAxes(AABB& _collider, Vec3& _velocity, std::span<const AABB> _blocking) {
	for (const AABB& box : _blocking)
		_velocity.y = box.CalculateYOffset(_collider, _velocity.y);
	_collider = _collider.Offset(0.0, _velocity.y, 0.0);

	for (const AABB& box : _blocking)
		_velocity.x = box.CalculateXOffset(_collider, _velocity.x);
	_collider = _collider.Offset(_velocity.x, 0.0, 0.0);

	for (const AABB& box : _blocking)
		_velocity.z = box.CalculateZOffset(_collider, _velocity.z);
	_collider = _collider.Offset(0.0, 0.0, _velocity.z);
}

// How far the collider still has to drop after a step-up, so the entity lands on
// the surface it stepped onto instead of hanging a step above it.
inline double SettleAfterStep(const AABB& _collider, double _stepHeight, std::span<const AABB> _blocking) {
	double settle = -_stepHeight;
	for (const AABB& box : _blocking)
		settle = box.CalculateYOffset(_collider, settle);
	return settle;
}

// What a swept move ran into. The caller turns these into whatever state it
// Server writes entity flags, the client feeds its own prediction.
struct SweepResult {
	bool collidedHorizontally = false;
	bool collidedVertically = false;
	bool onGround = false;
};

// Represents one movement step: optional sneak-edge clamping, a swept collision
// resolved axis by axis, then a step-up retry that is kept only if it covered
// more ground than being blocked did. Leaves `_collider` where the move ended
// and zeroes the components of `_velocity` that hit something.
//
// `_collect(box, out)` fills `out` with every block collider overlapping `box`;
// it is always called with `out` empty, so it may assign to it.
//
// This attempts to provide a single source for movement resolution.
// Entity::Move and the client's own prediction use it; attempts to mitigate
// as much as possible rubberbanding.
template <typename CollectFn>
SweepResult Sweep(AABB& _collider, Vec3& _velocity, float& _ySize, double _stepHeight, bool _clampSneak, bool _onGround,
                  CollectFn&& _collect) {
	const AABB originalCollider = _collider;
	std::vector<AABB> blocking;

	if (_clampSneak) {
		// Refuse to walk off a ledge while sneaking, one small step at a time.
		constexpr double STEP = 0.05;
		const auto groundBelow = [&](double _dx, double _dz) {
			blocking.clear();
			_collect(_collider.Offset(_dx, -1.0, _dz), blocking);
			return !blocking.empty();
		};

		while (_velocity.x != 0.0 && !groundBelow(_velocity.x, 0.0)) {
			if (_velocity.x < STEP && _velocity.x >= -STEP)
				_velocity.x = 0.0;
			else if (_velocity.x > 0.0)
				_velocity.x -= STEP;
			else
				_velocity.x += STEP;
		}
		while (_velocity.z != 0.0 && !groundBelow(0.0, _velocity.z)) {
			if (_velocity.z < STEP && _velocity.z >= -STEP)
				_velocity.z = 0.0;
			else if (_velocity.z > 0.0)
				_velocity.z -= STEP;
			else
				_velocity.z += STEP;
		}
	}

	// After the clamp, because that is the move actually being attempted.
	const Vec3 wanted = _velocity;

	blocking.clear();
	_collect(_collider.AddCoord(_velocity.x, _velocity.y, _velocity.z), blocking);
	ResolveAxes(_collider, _velocity, blocking);

	// Only the Y pass touches _velocity.y, so this is the same value it would
	// have had between the Y and X passes.
	const bool canStepUp = _onGround || (wanted.y != _velocity.y && wanted.y < 0.0);
	const bool blockedHorizontally = wanted.x != _velocity.x || wanted.z != _velocity.z;

	if (_stepHeight > 0.0 && canStepUp && (_clampSneak || _ySize < 0.05f) && blockedHorizontally) {
		// Retry the same movement from the original spot, lifted by a step.
		const Vec3 blockedMovement = _velocity;
		const AABB blockedCollider = _collider;

		_velocity = { wanted.x, _stepHeight, wanted.z };
		_collider = originalCollider;

		blocking.clear();
		_collect(_collider.AddCoord(_velocity.x, _velocity.y, _velocity.z), blocking);
		ResolveAxes(_collider, _velocity, blocking);
		_collider = _collider.Offset(0.0, SettleAfterStep(_collider, _stepHeight, blocking), 0.0);

		// Keep whichever attempt covered more ground.
		if (blockedMovement.x * blockedMovement.x + blockedMovement.z * blockedMovement.z >=
		    _velocity.x * _velocity.x + _velocity.z * _velocity.z) {
			_velocity = blockedMovement;
			_collider = blockedCollider;
		} else {
			const double fraction = _collider.minY - std::trunc(_collider.minY);
			if (fraction > 0.0)
				_ySize += float(fraction + 0.01);
		}
	}

	SweepResult result;
	result.collidedHorizontally = wanted.x != _velocity.x || wanted.z != _velocity.z;
	result.collidedVertically = wanted.y != _velocity.y;
	result.onGround = wanted.y != _velocity.y && wanted.y < 0.0;

	if (wanted.x != _velocity.x)
		_velocity.x = 0.0;
	if (wanted.y != _velocity.y)
		_velocity.y = 0.0;
	if (wanted.z != _velocity.z)
		_velocity.z = 0.0;

	return result;
}

} // namespace Movement
