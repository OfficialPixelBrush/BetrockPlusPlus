/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

// Lighter used by the main world.
#pragma once
#include "blocks/block_properties.h"
#include "chunk.h"
#include <climits>
#include <cstdint>
#include <numeric_structs.h>
#include <vector>

struct WorldManager;

enum class LightType : uint8_t {
	Sky,
	Block
};

// A light update entry covering a 3-D axis-aligned region [min, max] inclusive.
struct LightRegion {
	Int3 m_min{ 0, 0, 0 };
	Int3 m_max{ 0, 0, 0 };
	LightType m_type;

	// Returns true if [x1,y1,z1]->[x2,y2,z2] is contained within (or close
	// enough to merge into) this region, expanding it in-place when the volume
	// grows by <=2
	bool tryMerge(int x1, int y1, int z1, int x2, int y2, int z2) {
		if (x1 >= m_min.m_x && y1 >= m_min.m_y && z1 >= m_min.m_z && x2 <= m_max.m_x && y2 <= m_max.m_y && z2 <= m_max.m_z)
			return true;

		if (x1 < m_min.m_x - 1 || y1 < m_min.m_y - 1 || z1 < m_min.m_z - 1)
			return false;
		if (x2 > m_max.m_x + 1 || y2 > m_max.m_y + 1 || z2 > m_max.m_z + 1)
			return false;

		int oldVol = (m_max.m_x - m_min.m_x + 1) * (m_max.m_y - m_min.m_y + 1) * (m_max.m_z - m_min.m_z + 1);
		int nx1 = CrossPlatform::Math::min(m_min.m_x, x1), ny1 = CrossPlatform::Math::min(m_min.m_y, y1),
		    nz1 = CrossPlatform::Math::min(m_min.m_z, z1);
		int nx2 = CrossPlatform::Math::max(m_max.m_x, x2), ny2 = CrossPlatform::Math::max(m_max.m_y, y2),
		    nz2 = CrossPlatform::Math::max(m_max.m_z, z2);
		int newVol = (nx2 - nx1 + 1) * (ny2 - ny1 + 1) * (nz2 - nz1 + 1);
		if (newVol - oldVol > 2)
			return false;

		m_min = { nx1, ny1, nz1 };
		m_max = { nx2, ny2, nz2 };
		return true;
	}
};

struct UnlightUpdate {
	Int3 m_pos{ 0, 0, 0 };
	LightType m_type;
	int m_val;
};

// 3x3 grid of chunk pointers centered on a chunk coordinate.
struct ChunkCache {
	Chunk* m_grid[3][3] = {};
	int m_cx = INT_MIN, m_cz = INT_MIN; // chunk coords of center (grid[1][1])

	// Fetch chunk at chunk-coord (tcx, tcz) from the grid.
	Chunk* get(int tcx, int tcz) const {
		int dx = tcx - m_cx, dz = tcz - m_cz;
		if (dx < -1 || dx > 1 || dz < -1 || dz > 1)
			return nullptr;
		return m_grid[dx + 1][dz + 1];
	}

	// Refresh the entire 3x3 grid around (ncx, ncz) if the center has changed.
	void refresh(int ncx, int ncz, WorldManager& world);
};

struct Lighter {
public:
	void propagateLightAt(int x, int y, int z, LightType type, WorldManager& world, ChunkCache& cache);
	void unlightAt(int x, int y, int z, LightType type, WorldManager& world);

	// Process up to `maxIterations` light-queue entries this call.
	bool processLightQueue(WorldManager& world, int maxIterations = INT_MAX);

	// Schedule a single-block update; bypasses merge (used for BFS fan-out).
	void scheduleLightUpdate(Int3 pos, LightType type) {
		if (pos.m_y < 0 || pos.m_y >= CHUNK_HEIGHT)
			return;
		if (lightQueue.size() < 1000000)
			lightQueue.push_back({ pos, pos, type });
	}

	// Schedule a region [mn, mx] update with merge/dedup
	void scheduleLightRegion(Int3 mn, Int3 mx, LightType type) {
		mn.m_y = CrossPlatform::Math::max(mn.m_y, 0);
		mx.m_y = CrossPlatform::Math::min(mx.m_y, CHUNK_HEIGHT - 1);
		if (mn.m_y > mx.m_y)
			return;

		size_t checkCount = CrossPlatform::Math::min(lightQueue.size(), size_t(5));
		for (size_t i = 0; i < checkCount; ++i) {
			LightRegion& r = lightQueue[size_t(lightQueue.size() - 1 - i)];
			if (r.m_type != type)
				continue;
			if (r.tryMerge(mn.m_x, mn.m_y, mn.m_z, mx.m_x, mx.m_y, mx.m_z))
				return;
		}

		if (lightQueue.size() >= 1000000)
			return;
		lightQueue.push_back({ mn, mx, type });
	}

private:
	std::vector<LightRegion> lightQueue;
	std::vector<UnlightUpdate> unlightQueue;
	int processingDepth = 0;

	// Persistent cache for unlightAt
	ChunkCache m_unlightCache;
};