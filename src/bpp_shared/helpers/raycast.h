/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Tiago F <tacf>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include "../direction.h"
#include "ray_aabb.h"
#include "world.h"
#include <cmath>
#include <limits>

enum RayCastMode {
	IGNORE_FLUIDS,
	ACCEPT_SOURCES,
	ACCEPT_ANY
};

struct RayCastResult {
	bool hit = false;
	Block hitBlock = {};
	Int3 blockPosition = {};
	Direction::Value face = Direction::Value::None;
};

// Name is a bit obvious isn't it?
namespace Raycast {

// Whether the fluid block should be treated as solid for this raycast mode.
inline bool ShouldConsiderFluid(RayCastMode _mode, uint8_t _meta) {
	switch (_mode) {
	case IGNORE_FLUIDS:
		return false;
	case ACCEPT_SOURCES:
		return _meta == 0;
	case ACCEPT_ANY:
	default:
		return true;
	}
}

inline RayCastResult Raycast(WorldManager& _world, Vec3 _startPos, Vec3 _endPos, RayCastMode _mode) {
	RayCastResult result;

	Vec3 dir = _endPos - _startPos;
	double dist = dir.Length();
	if (dist < 0.000001)
		return result; // start == end, nothing to hit

	dir = dir / dist;

	int x = MathHelper::FloorDouble(_startPos.x);
	int y = MathHelper::FloorDouble(_startPos.y);
	int z = MathHelper::FloorDouble(_startPos.z);

	int stepX = dir.x > 0 ? 1 : -1, stepY = dir.y > 0 ? 1 : -1, stepZ = dir.z > 0 ? 1 : -1;

	auto tMaxFor = [](double _origin, int _cell, int _step, double _dirComp) {
		if (_dirComp == 0)
			return std::numeric_limits<double>::infinity();
		double boundary = (_step > 0) ? (_cell + 1) : _cell;
		return (boundary - _origin) / _dirComp;
	};
	double tMaxX = tMaxFor(_startPos.x, x, stepX, dir.x);
	double tMaxY = tMaxFor(_startPos.y, y, stepY, dir.y);
	double tMaxZ = tMaxFor(_startPos.z, z, stepZ, dir.z);
	double tDeltaX = (dir.x != 0) ? std::abs(1.0 / dir.x) : std::numeric_limits<double>::infinity();
	double tDeltaY = (dir.y != 0) ? std::abs(1.0 / dir.y) : std::numeric_limits<double>::infinity();
	double tDeltaZ = (dir.z != 0) ? std::abs(1.0 / dir.z) : std::numeric_limits<double>::infinity();

	while (true) {
		BlockType id = _world.GetBlockId({ x, y, z });

		// IsRealBlock here attempts to solve out of bounds issues.
		if (IsRealBlock(id)) {
			const auto& props = Blocks::blockProperties[id];
			bool consider = true;

			if (props.material.isLiquid) {
				uint8_t meta = _world.GetMetadata({ x, y, z });
				consider = ShouldConsiderFluid(_mode, meta);
			}

			if (consider) {
				uint8_t meta = _world.GetMetadata({ x, y, z });
				AABB localBox = Blocks::blockBehaviors[id].getRayBounds(meta);

				// Zero volume shapes
				if (localBox.maxX > localBox.minX && localBox.maxY > localBox.minY && localBox.maxZ > localBox.minZ) {
					AABB worldBox = localBox.Offset(x, y, z);

					double hitT;
					Direction::Value hitFace;
					if (ClipRayAABB(_startPos, dir, dist, worldBox, hitT, hitFace)) {
						result.hit = true;
						result.hitBlock = { id, meta };
						result.blockPosition = { x, y, z };
						result.face = hitFace;
						return result;
					}
				}
			}
		}

		// Advance to the next candidate cell along the ray
		double nextT;
		if (tMaxX <= tMaxY && tMaxX <= tMaxZ) {
			nextT = tMaxX;
			x += stepX;
			tMaxX += tDeltaX;
		} else if (tMaxY <= tMaxZ) {
			nextT = tMaxY;
			y += stepY;
			tMaxY += tDeltaY;
		} else {
			nextT = tMaxZ;
			z += stepZ;
			tMaxZ += tDeltaZ;
		}

		if (nextT > dist)
			break;
	}

	return result; // no hit within range
}

} // namespace Raycast