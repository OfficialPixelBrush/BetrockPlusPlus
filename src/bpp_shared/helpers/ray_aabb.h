/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Tiago F <tacf>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

// The ray/box geometry on its own, split out of raycast.h so the client can use
// it without dragging othe objects like WorldManager and thread pool.

#pragma once
#include "../direction.h"
#include "AABB.h"
#include <cmath>
#include <numeric_structs.h>

namespace Raycast {

inline bool ClipRayAABB(const Vec3& _origin, const Vec3& _dir, double _maxDist, const AABB& _box, double& _outT,
                        Direction::Value& _outFace) {
	double originArr[3] = { _origin.x, _origin.y, _origin.z };
	double dirArr[3] = { _dir.x, _dir.y, _dir.z };
	double boxMin[3] = { _box.minX, _box.minY, _box.minZ };
	double boxMax[3] = { _box.maxX, _box.maxY, _box.maxZ };

	double tMin = 0.0;
	double tMax = _maxDist;
	int hitAxis = -1;

	for (int axis = 0; axis < 3; ++axis) {
		double o = originArr[axis];
		double d = dirArr[axis];
		double mn = boxMin[axis];
		double mx = boxMax[axis];

		if (std::abs(d) < 1e-12) {
			// Ray is parallel to this slab; must already be inside it
			if (o < mn || o > mx)
				return false;
			continue;
		}

		double invD = 1.0 / d;
		double t1 = (mn - o) * invD;
		double t2 = (mx - o) * invD;
		if (t1 > t2)
			std::swap(t1, t2);

		if (t1 > tMin) {
			tMin = t1;
			hitAxis = axis;
		}
		if (t2 < tMax)
			tMax = t2;

		if (tMin > tMax)
			return false;
	}

	// hitAxis == -1 means the ray origin started inside the box on every axis
	if (hitAxis == -1)
		return false;

	_outT = tMin;

	double d = dirArr[hitAxis];
	switch (hitAxis) {
	case 0:
		_outFace = (d > 0) ? Direction::Value::West : Direction::Value::East;
		break;
	case 1:
		_outFace = (d > 0) ? Direction::Value::Down : Direction::Value::Up;
		break;
	default:
		_outFace = (d > 0) ? Direction::Value::North : Direction::Value::South;
		break;
	}
	return true;
}

} // namespace Raycast
