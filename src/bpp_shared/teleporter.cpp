/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "world/world.h"
#include "constants.h"
#include <algorithm>

namespace Teleporter {

static constexpr int EXISTING_PORTAL_SEARCH_RADIUS = 128;
static constexpr int NEW_PORTAL_SEARCH_RADIUS = 16;

bool PlaceInExistingPortal(WorldManager& _world, Entity& _entity);
bool CreatePortal(WorldManager& _world, Entity& _entity);

namespace {
// TODO: Turn facing into an Enum, so we can avoid this sorta stuff, especially because a lot of parts of the code,
// especially redstone manager have their own unique ways of doing this themselves, which is stupid.
Int2 FacingToAxis(int _facing) {
	int ax = _facing % 2;
	int az = 1 - ax;
	if (_facing % 4 >= 2) {
		ax = -ax;
		az = -az;
	}
	return { ax, az };
}

bool HasRoomForPortal(WorldManager& _world, Int3 _base, Int2 _axis, int _depth) {
	for (int d = 0; d < _depth; d++) {
		for (int w = 0; w < 4; w++) {
			for (int h = -1; h < 4; h++) {
				Int3 p = { _base.x + (w - 1) * _axis.x + d * _axis.z, _base.y + h,
					       _base.z + (w - 1) * _axis.z - d * _axis.x };
				if (h < 0) {
					if (!_world.GetMaterial(p).isSolid)
						return false;
				} else {
					if (!_world.IsAirBlock(p))
						return false;
				}
			}
		}
	}
	return true;
}

bool FindPortalSite(WorldManager& _world, const Vec3& _origin, int _startFacing, int _facingCount, int _depth,
                    Int3& _bestPos, int& _bestFacing) {
	const Int2 center = {
		MathHelper::FloorDouble(_origin.x),
		MathHelper::FloorDouble(_origin.z)
	};
	double closest = -1.0;

	for (int x = center.x - NEW_PORTAL_SEARCH_RADIUS; x <= center.x + NEW_PORTAL_SEARCH_RADIUS; x++) {
		const double dx = x + 0.5 - _origin.x;
		for (int z = center.z - NEW_PORTAL_SEARCH_RADIUS; z <= center.z + NEW_PORTAL_SEARCH_RADIUS; z++) {
			const double dz = z + 0.5 - _origin.z;

			for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
				if (!_world.IsAirBlock({ x, y, z }))
					continue;

				// Drop to the lowest air block of this pocket
				while (y > 0 && _world.IsAirBlock({ x, y - 1, z }))
					y--;

				for (int f = _startFacing; f < _startFacing + _facingCount; f++) {
					int facing = _facingCount == 4 ? f % 4 : f % 2;
					// Vanilla quirk: one blocked facing skips the remaining facings at this Y
					if (!HasRoomForPortal(_world, { x, y, z }, FacingToAxis(facing), _depth))
						break;

					const double dy = y + 0.5 - _origin.y;
					const double distSq = dx * dx + dy * dy + dz * dz;
					if (closest < 0.0 || distSq < closest) {
						closest = distSq;
						_bestPos = { x, y, z };
						_bestFacing = facing;
					}
				}
			}
		}
	}

	return closest >= 0.0;
}
} // namespace

void PlaceAtValidTransferSpot(WorldManager& _world, Entity& _entity) {
	if (!PlaceInExistingPortal(_world, _entity)) {
		CreatePortal(_world, _entity);
		PlaceInExistingPortal(_world, _entity);
	}
}

bool PlaceInExistingPortal(WorldManager& _world, Entity& _entity) {
	const Vec3& pos = _entity.position;
	const int ex = MathHelper::FloorDouble(pos.x);
	const int ez = MathHelper::FloorDouble(pos.z);
	const int minX = ex - EXISTING_PORTAL_SEARCH_RADIUS, maxX = ex + EXISTING_PORTAL_SEARCH_RADIUS;
	const int minZ = ez - EXISTING_PORTAL_SEARCH_RADIUS, maxZ = ez + EXISTING_PORTAL_SEARCH_RADIUS;

	double closest = -1.0;
	Int3 portalPos = {};

	for (int cx = minX >> 4; cx <= maxX >> 4; cx++) {
		for (int cz = minZ >> 4; cz <= maxZ >> 4; cz++) {
			Chunk* chunk = _world.GetChunkRaw({ cx, cz });
			if (!_world.IsChunkValid(chunk))
				continue;

			// Clip the search box to this chunk
			const int x0 = std::max(minX, cx * CHUNK_WIDTH), x1 = std::min(maxX, cx * CHUNK_WIDTH + (CHUNK_WIDTH-1));
			const int z0 = std::max(minZ, cz * CHUNK_WIDTH), z1 = std::min(maxZ, cz * CHUNK_WIDTH + (CHUNK_WIDTH-1));

			for (int x = x0; x <= x1; x++) {
				double dx = x + 0.5 - pos.x;
				for (int z = z0; z <= z1; z++) {
					double dz = z + 0.5 - pos.z;
					for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
						if (chunk->GetBlock({ x & 15, y, z & 15 }) != BLOCK_NETHER_PORTAL)
							continue;
						while (y > 0 && chunk->GetBlock({ x & 15, y - 1, z & 15 }) == BLOCK_NETHER_PORTAL)
							y--;

						const double dy = y + 0.5 - pos.y;
						const double distSq = dx * dx + dy * dy + dz * dz;

						bool better = closest < 0.0 || distSq < closest ||
						              (distSq == closest &&
						               (x < portalPos.x ||
						                (x == portalPos.x &&
						                 (z < portalPos.z || (z == portalPos.z && y > portalPos.y)))));
						if (better) {
							closest = distSq;
							portalPos = { x, y, z };
						}
					}
				}
			}
		}
	}

	// Didn't find anything
	if (closest < 0.0)
		return false;

	// TP our entity to the center of the portal
	const Direction::Value neighbors[4] = { Direction::Value::West, Direction::Value::East, Direction::Value::North,
		                              Direction::Value::South };
	const double offsets[4][2] = { { -0.5, 0.0 }, { 0.5, 0.0 }, { 0.0, -0.5 }, { 0.0, 0.5 } };

	Vec3 targetPos = portalPos + 0.5;
	for (int i = 0; i < 4; i++) {
		if (_world.GetBlockId(portalPos.WithOffset(neighbors[i])) == BLOCK_NETHER_PORTAL) {
			targetPos.x += offsets[i][0];
			targetPos.z += offsets[i][1];
		}
	}

	_entity.velocity = {};
	_entity.Teleport(targetPos, { _entity.rotationYaw, 0.0f });
	return true;
}

bool CreatePortal(WorldManager& _world, Entity& _entity) {
	Int3 bestPos = {
		MathHelper::FloorDouble(_entity.position.x),
		MathHelper::FloorDouble(_entity.position.y),
		MathHelper::FloorDouble(_entity.position.z)
	};
	int bestFacing = 0;
	const int startFacing = _world.rand.NextInt(4);

	bool foundSite = FindPortalSite(_world, _entity.position, startFacing, 4, 3, bestPos, bestFacing) ||
	                 FindPortalSite(_world, _entity.position, startFacing, 2, 1, bestPos, bestFacing);

	Int2 axis = FacingToAxis(bestFacing);
	// TODO: Couldn't bestPos be reused for this, since it's only used once for the operation above?
	Int3 base = bestPos;

	if (!foundSite) {
		base.y = std::clamp(base.y, 70, 118);
		for (int side = -1; side <= 1; side++) {
			for (int w = 1; w < 3; w++) {
				for (int h = -1; h < 3; h++) {
					Int3 p = { base.x + (w - 1) * axis.x + side * axis.z, base.y + h,
						       base.z + (w - 1) * axis.z - side * axis.x };
					_world.SetBlock(p, h < 0 ? BLOCK_OBSIDIAN : BLOCK_AIR);
				}
			}
		}
	}

	auto framePos = [&](int _w, int _h) -> Int3 {
		return { base.x + (_w - 1) * axis.x, base.y + _h, base.z + (_w - 1) * axis.z };
	};

	// Place everything without neighbour updates
	for (int w = 0; w < 4; w++) {
		for (int h = -1; h < 4; h++) {
			bool isFrame = w == 0 || w == 3 || h == -1 || h == 3;
			_world.SetBlock(framePos(w, h), isFrame ? BLOCK_OBSIDIAN : BLOCK_NETHER_PORTAL, /*meta=*/0,
			                /*keepTileEntity=*/false, /*updateNeighbors=*/false);
		}
	}

	// Notify neighbors
	for (int w = 0; w < 4; w++) {
		for (int h = -1; h < 4; h++) {
			Int3 p = framePos(w, h);
			_world.NotifyNeighborsOfUpdate(p, _world.GetBlockId(p));
		}
	}

	return true;
}
} // namespace Teleporter