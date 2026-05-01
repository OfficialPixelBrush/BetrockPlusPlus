/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

// Lighter used by the main world.
#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>
#include <climits>
#include <numeric_structs.h>
#include "blocks/block_properties.h"
#include "chunk.h"

struct WorldManager;

enum class LightType : uint8_t { Sky, Block };

// A light update entry covering a 3-D axis-aligned region [min, max] inclusive.
struct LightRegion {
    Int3 min{ 0, 0, 0 };
    Int3 max{ 0, 0, 0 };
    LightType type;

    // Returns true if [x1,y1,z1]->[x2,y2,z2] is contained within (or close
    // enough to merge into) this region, expanding it in-place when the volume
    // grows by <=2
    bool tryMerge(int x1, int y1, int z1, int x2, int y2, int z2) {
        if (x1 >= min.x && y1 >= min.y && z1 >= min.z &&
            x2 <= max.x && y2 <= max.y && z2 <= max.z)
            return true;

        if (x1 < min.x - 1 || y1 < min.y - 1 || z1 < min.z - 1) return false;
        if (x2 > max.x + 1 || y2 > max.y + 1 || z2 > max.z + 1) return false;

        int oldVol = (max.x - min.x + 1) * (max.y - min.y + 1) * (max.z - min.z + 1);
        int nx1 = std::min(min.x, x1), ny1 = std::min(min.y, y1), nz1 = std::min(min.z, z1);
        int nx2 = std::max(max.x, x2), ny2 = std::max(max.y, y2), nz2 = std::max(max.z, z2);
        int newVol = (nx2 - nx1 + 1) * (ny2 - ny1 + 1) * (nz2 - nz1 + 1);
        if (newVol - oldVol > 2) return false;

        min = { nx1, ny1, nz1 };
        max = { nx2, ny2, nz2 };
        return true;
    }
};

struct UnlightUpdate { Int3 pos{ 0, 0, 0 }; LightType type; int val; };

// 3x3 grid of chunk pointers centered on a chunk coordinate.
struct ChunkCache {
    Chunk* grid[3][3] = {};
    int cx = INT_MIN, cz = INT_MIN; // chunk coords of center (grid[1][1])

    // Fetch chunk at chunk-coord (tcx, tcz) from the grid.
    Chunk* get(int tcx, int tcz) const {
        int dx = tcx - cx, dz = tcz - cz;
        if (dx < -1 || dx > 1 || dz < -1 || dz > 1) return nullptr;
        return grid[dx + 1][dz + 1];
    }

    // Refresh the entire 3x3 grid around (ncx, ncz) if the center has changed.
    void refresh(int ncx, int ncz, WorldManager& world);
};

struct Lighter {
public:
    void propagateLightAt(int x, int y, int z, LightType type, WorldManager& world, ChunkCache& cache);
    void unlightAt(int x, int y, int z, LightType type, WorldManager& world);

    // Process up to `maxIterations` light-queue entries this call.
    // Pass INT_MAX (or omit) to drain everything — used during startup
    // where the old "finish in one go" behaviour is still wanted.
    // Returns true if work remains after the budget is exhausted.
    bool processLightQueue(WorldManager& world, int maxIterations = INT_MAX);

    // Schedule a single-block update — bypasses merge (used for BFS fan-out).
    void scheduleLightUpdate(Int3 pos, LightType type) {
        if (pos.y < 0 || pos.y >= CHUNK_HEIGHT) return;
        if (lightQueue.size() < 1000000)
            lightQueue.push_back({ pos, pos, type });
    }

    // Schedule a region [mn, mx] update with merge/dedup — used for seeding.
    void scheduleLightRegion(Int3 mn, Int3 mx, LightType type) {
        mn.y = std::max(mn.y, 0);
        mx.y = std::min(mx.y, CHUNK_HEIGHT - 1);
        if (mn.y > mx.y) return;

        size_t checkCount = std::min(lightQueue.size(), size_t(5));
        for (size_t i = 0; i < checkCount; ++i) {
            LightRegion& r = lightQueue[size_t(lightQueue.size() - 1 - i)];
            if (r.type != type) continue;
            if (r.tryMerge(mn.x, mn.y, mn.z, mx.x, mx.y, mx.z))
                return;
        }

        if (lightQueue.size() >= 1000000) return;
        lightQueue.push_back({ mn, mx, type });
    }

private:
    std::vector<LightRegion> lightQueue;
    std::vector<UnlightUpdate> unlightQueue;
    int processingDepth = 0;
};