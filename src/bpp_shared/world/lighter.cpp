/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "lighter.h"
#include "blocks.h"
#include "constants.h"
#include "world.h"
#include <cstring>

// ChunkCache so we don't have to do map lookups for every neighbor access during light propagation.
// Reuses chunks from the previous window whenever they overlap the new one.
void ChunkCache::Refresh(int _ncx, int _ncz, WorldManager& _world) {
	if (_ncx == cx && _ncz == cz)
		return;

	Chunk* oldGrid[3][3];
	std::memcpy(oldGrid, grid, sizeof(grid));
	int oldCx = cx, oldCz = cz;

	cx = _ncx;
	cz = _ncz;

	for (int dx = -1; dx <= 1; ++dx) {
		for (int dz = -1; dz <= 1; ++dz) {
			int tcx = _ncx + dx, tcz = _ncz + dz;
			int odx = tcx - oldCx, odz = tcz - oldCz;

			// Already held from the previous window? Reuse it as-is.
			Chunk* c = (odx >= -1 && odx <= 1 && odz >= -1 && odz <= 1) ? oldGrid[odx + 1][odz + 1] : nullptr;

			if (!c) {
				Chunk* fetched = _world.GetChunkRaw({ tcx, tcz });
				c = (fetched && fetched->state.load(std::memory_order_acquire) >= ChunkState::Generated) ? fetched
				                                                                                         : nullptr;
			}

			grid[dx + 1][dz + 1] = c;
		}
	}
}

static inline int GetLightDirect(Chunk* _chunk, int _lx, int _y, int _lz, LightType _type) {
	if (!_chunk || _y < 0 || _y >= CHUNK_HEIGHT)
		return 0;
	return (_type == LightType::Sky) ? _chunk->GetSkyLight({ _lx, _y, _lz }) : _chunk->GetBlockLight({ _lx, _y, _lz });
}

void Lighter::PropagateLightAt(int _x, int _y, int _z, LightType _type, WorldManager& _world, ChunkCache& _cache) {
	if (_y < 0 || _y >= CHUNK_HEIGHT)
		return;

	int cx = _x >> 4, cz = _z >> 4;

	// Refresh the 3x3 cache only when we cross a chunk boundary.
	_cache.Refresh(cx, cz, _world);

	Chunk* chunk = _cache.grid[1][1]; // Center
	if (!chunk)
		return;

	int lx = _x & 15, lz = _z & 15;
	BlockType blockId = chunk->GetBlock({ lx, _y, lz });
	int opacity = Blocks::blockProperties[blockId].lightOpacity;
	if (opacity == 0)
		opacity = 1;

	// Pick neighbor chunk pointers
	Chunk* cxn = (lx == 0) ? _cache.grid[0][1] : chunk;
	Chunk* cxp = (lx == 15) ? _cache.grid[2][1] : chunk;
	Chunk* czn = (lz == 0) ? _cache.grid[1][0] : chunk;
	Chunk* czp = (lz == 15) ? _cache.grid[1][2] : chunk;

	int lxn = (lx == 0) ? 15 : lx - 1;
	int lxp = (lx == 15) ? 0 : lx + 1;
	int lzn = (lz == 0) ? 15 : lz - 1;
	int lzp = (lz == 15) ? 0 : lz + 1;

	int newVal = 0;

	if (_type == LightType::Sky) {
		if (chunk->CanBlockSeeSky({ lx, _y, lz })) {
			newVal = 15;
		} else if (opacity < 15) {
			int best = 0;
			best = CrossPlatform::Math::Max(best, GetLightDirect(cxn, lxn, _y, lz, _type));
			best = CrossPlatform::Math::Max(best, GetLightDirect(cxp, lxp, _y, lz, _type));
			best = CrossPlatform::Math::Max(best, GetLightDirect(chunk, lx, _y - 1, lz, _type));
			best = CrossPlatform::Math::Max(best, GetLightDirect(chunk, lx, _y + 1, lz, _type));
			best = CrossPlatform::Math::Max(best, GetLightDirect(czn, lx, _y, lzn, _type));
			best = CrossPlatform::Math::Max(best, GetLightDirect(czp, lx, _y, lzp, _type));
			newVal = CrossPlatform::Math::Max(0, best - opacity);
		}
		int oldVal = chunk->GetSkyLight({ lx, _y, lz });
		if (oldVal == newVal)
			return;
		chunk->SetSkyLight({ lx, _y, lz }, uint8_t(newVal));
		// Call a block update on the block that had its lighting updated
		// Beta doesn't have a direct on lighting change packet for server -> client
		// So this is what we have to resort to
		// Technically, this doesn't work for any light update that doesn't change more than 9 blocks but its good enough
		if (_world.onBlockUpdate)
			_world.onBlockUpdate(PendingBlock{ .block{ BlockType(blockId), chunk->GetMeta({ lx, _y, lz }) },
			                                  .blockPos{ _x, _y, _z },
			                                  .light{ chunk->GetBlockLight({ lx, _y, lz }), uint8_t(newVal) } },
			                    chunk->cpos);
	} else {
		int emitted = Blocks::blockProperties[blockId].lightEmission;
		if (opacity < 15) {
			int best = 0;
			best = CrossPlatform::Math::Max(best, GetLightDirect(cxn, lxn, _y, lz, _type));
			best = CrossPlatform::Math::Max(best, GetLightDirect(cxp, lxp, _y, lz, _type));
			best = CrossPlatform::Math::Max(best, GetLightDirect(chunk, lx, _y - 1, lz, _type));
			best = CrossPlatform::Math::Max(best, GetLightDirect(chunk, lx, _y + 1, lz, _type));
			best = CrossPlatform::Math::Max(best, GetLightDirect(czn, lx, _y, lzn, _type));
			best = CrossPlatform::Math::Max(best, GetLightDirect(czp, lx, _y, lzp, _type));
			newVal = CrossPlatform::Math::Max(emitted, best - opacity);
		} else {
			newVal = emitted;
		}
		int oldVal = chunk->GetBlockLight({ lx, _y, lz });
		if (oldVal == newVal)
			return;
		chunk->SetBlockLight({ lx, _y, lz }, uint8_t(newVal));
		if (_world.onBlockUpdate)
			_world.onBlockUpdate(
			    PendingBlock{
			        .block{ BlockType(blockId), chunk->GetMeta({ lx, _y, lz }) },
			        .blockPos{ _x, _y, _z },
			        .light{ uint8_t(newVal), chunk->GetSkyLight({ lx, _y, lz }) },
			    },
			    chunk->cpos);
	}

	// Propagate to neighbors
	auto maybeQueue = [&](int _nx, int _ny, int _nz, Chunk* _nc, int _nlx, int _nlz) {
		if (_ny < 0 || _ny >= CHUNK_HEIGHT || !_nc)
			return;
		int expected = CrossPlatform::Math::Max(0, newVal - 1);
		if (GetLightDirect(_nc, _nlx, _ny, _nlz, _type) < expected && lightQueue.size() < 1000000)
			lightQueue.push_back({ { _nx, _ny, _nz }, { _nx, _ny, _nz }, _type });
	};

	maybeQueue(_x - 1, _y, _z, cxn, lxn, lz);
	maybeQueue(_x + 1, _y, _z, cxp, lxp, lz);
	maybeQueue(_x, _y - 1, _z, chunk, lx, lz);
	maybeQueue(_x, _y + 1, _z, chunk, lx, lz);
	maybeQueue(_x, _y, _z - 1, czn, lx, lzn);
	maybeQueue(_x, _y, _z + 1, czp, lx, lzp);
}

void Lighter::UnlightAt(int _x, int _y, int _z, LightType _type, WorldManager& _world) {
	// Separate function for this since beta's lighting engine can't decrease light values in place
	if (_y < 0 || _y >= CHUNK_HEIGHT)
		return;

	unlightCache.Refresh(_x >> 4, _z >> 4, _world);
	Chunk* chunk = unlightCache.grid[1][1];
	if (!chunk)
		return;

	int lx = _x & 15, lz = _z & 15;
	int oldVal = (_type == LightType::Sky) ? chunk->GetSkyLight({ lx, _y, lz }) : chunk->GetBlockLight({ lx, _y, lz });
	if (oldVal == 0)
		return;

	if (_type == LightType::Sky)
		chunk->SetSkyLight({ lx, _y, lz }, 0);
	else
		chunk->SetBlockLight({ lx, _y, lz }, 0);
	if (_world.onBlockUpdate)
		_world.onBlockUpdate(
		    PendingBlock{
		        .block{ BlockType(chunk->GetBlock({ lx, _y, lz })), chunk->GetMeta({ lx, _y, lz }) },
		        .blockPos{ _x, _y, _z },
		        // Whichever type we just unlit was just set to 0 above
		        .light{ (_type == LightType::Block) ? uint8_t(0) : chunk->GetBlockLight({ lx, _y, lz }),
		                (_type == LightType::Sky) ? uint8_t(0) : chunk->GetSkyLight({ lx, _y, lz }) },
		    },
		    chunk->cpos);

	unlightQueue.push_back({ { _x, _y, _z }, _type, oldVal });

	// Drain everything at once since there isn't a lot of unlighting events
	while (!unlightQueue.empty()) {
		auto [pos, t, val] = unlightQueue.back();
		unlightQueue.pop_back();

		unlightCache.Refresh(pos.x >> 4, pos.z >> 4, _world);

		const int ndx[] = { -1, 1, 0, 0, 0, 0 };
		const int ndy[] = { 0, 0, -1, 1, 0, 0 };
		const int ndz[] = { 0, 0, 0, 0, -1, 1 };
		for (int i = 0; i < 6; ++i) {
			int nx = pos.x + ndx[i];
			int ny = pos.y + ndy[i];
			int nz = pos.z + ndz[i];
			if (ny < 0 || ny >= CHUNK_HEIGHT)
				continue;

			int ncx = nx >> 4, ncz = nz >> 4;
			int nlx = nx & 15, nlz = nz & 15;
			int dx = ncx - unlightCache.cx, dz = ncz - unlightCache.cz;
			Chunk* nc = (dx >= -1 && dx <= 1 && dz >= -1 && dz <= 1) ? unlightCache.grid[dx + 1][dz + 1]
			                                                         : _world.GetChunkRaw({ ncx, ncz });
			if (!nc)
				continue;

			int nVal = (t == LightType::Sky) ? nc->GetSkyLight({ nlx, ny, nlz }) : nc->GetBlockLight({ nlx, ny, nlz });
			if (nVal == 0)
				continue;

			if (nVal < val) {
				if (t == LightType::Sky)
					nc->SetSkyLight({ nlx, ny, nlz }, 0);
				else
					nc->SetBlockLight({ nlx, ny, nlz }, 0);
				if (_world.onBlockUpdate)
					_world.onBlockUpdate(
					    PendingBlock{
					        .block{ BlockType(nc->GetBlock({ nlx, ny, nlz })), nc->GetMeta({ nlx, ny, nlz }) },
					        .blockPos{ nx, ny, nz },
					        .light{ nc->GetBlockLight({ nlx, ny, nlz }), nc->GetSkyLight({ nlx, ny, nlz }) },
					    },
					    nc->cpos);
				unlightQueue.push_back({ { nx, ny, nz }, t, nVal });
				// Always re-queue for re-light
				ScheduleLightUpdate({ nx, ny, nz }, t);
			} else {
				// Neighbor is at least as bright
				ScheduleLightUpdate({ nx, ny, nz }, t);
			}
		}
	}
}

bool Lighter::ProcessLightQueue(WorldManager& _world, int _maxIterations) {
	if (processingDepth >= 50)
		return !lightQueue.empty();
	++processingDepth;

	ChunkCache cache;
	int iters = 0;

	while (!lightQueue.empty() && iters < _maxIterations) {
		LightRegion region = lightQueue.back();
		lightQueue.pop_back();
		++iters;

		int dx = region.max.x - region.min.x + 1;
		int dy = region.max.y - region.min.y + 1;
		int dz = region.max.z - region.min.z + 1;
		if (dx * dy * dz > 32768)
			continue;

		region.min.y = CrossPlatform::Math::Max(region.min.y, 0);
		region.max.y = CrossPlatform::Math::Min(region.max.y, CHUNK_HEIGHT - 1);

		for (int x = region.min.x; x <= region.max.x; ++x)
			for (int z = region.min.z; z <= region.max.z; ++z)
				for (int y = region.min.y; y <= region.max.y; ++y)
					PropagateLightAt(x, y, z, region.type, _world, cache);
	}

	--processingDepth;
	if (lightQueue.empty())
		lightQueue.shrink_to_fit();

	return !lightQueue.empty();
}