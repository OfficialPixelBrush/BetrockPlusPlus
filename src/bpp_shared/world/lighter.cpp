/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "lighter.h"
#include "blocks.h"
#include "world.h"

// ChunkCache
void ChunkCache::refresh(int ncx, int ncz, WorldManager& world) {
    if (ncx == cx && ncz == cz) return;
    cx = ncx; cz = ncz;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz) {
            Chunk* c = world.getChunkRaw({ ncx + dx, ncz + dz });
            grid[dx + 1][dz + 1] = (c && c->state.load(std::memory_order_acquire) >= ChunkState::Generated) ? c : nullptr;
        }
}

// Helpers
static inline int getLightDirect(Chunk* chunk, int lx, int y, int lz, LightType type) {
    if (!chunk || y < 0 || y >= 128) return 0;
    return (type == LightType::Sky)
        ? chunk->getSkyLight({ lx, y, lz })
        : chunk->getBlockLight({ lx, y, lz });
}

void Lighter::propagateLightAt(int x, int y, int z, LightType type, WorldManager& world, ChunkCache& cache) {
    if (y < 0 || y >= CHUNK_HEIGHT) return;

    int cx = x >> 4, cz = z >> 4;

    // Refresh the 3x3 cache only when we cross a chunk boundary.
    cache.refresh(cx, cz, world);

    Chunk* chunk = cache.grid[1][1]; // center
    if (!chunk) return;

    int lx = x & 15, lz = z & 15;
    BlockType blockId = chunk->getBlock({ lx, y, lz });
    int opacity = Blocks::blockProperties[blockId].lightOpacity;
    if (opacity == 0) opacity = 1;

    // Pick neighbor chunk pointers directly from the cache (avoids map lookups)
    Chunk* cxn = (lx == 0) ? cache.grid[0][1] : chunk;
    Chunk* cxp = (lx == 15) ? cache.grid[2][1] : chunk;
    Chunk* czn = (lz == 0) ? cache.grid[1][0] : chunk;
    Chunk* czp = (lz == 15) ? cache.grid[1][2] : chunk;

    int lxn = (lx == 0) ? 15 : lx - 1;
    int lxp = (lx == 15) ? 0 : lx + 1;
    int lzn = (lz == 0) ? 15 : lz - 1;
    int lzp = (lz == 15) ? 0 : lz + 1;

    int newVal = 0;

    if (type == LightType::Sky) {
        if (chunk->canBlockSeeSky({ lx, y, lz })) {
            newVal = 15;
        }
        else if (opacity < 15) {
            int best = 0;
            best = std::max(best, getLightDirect(cxn, lxn, y, lz, type));
            best = std::max(best, getLightDirect(cxp, lxp, y, lz, type));
            best = std::max(best, getLightDirect(chunk, lx, y - 1, lz, type));
            best = std::max(best, getLightDirect(chunk, lx, y + 1, lz, type));
            best = std::max(best, getLightDirect(czn, lx, y, lzn, type));
            best = std::max(best, getLightDirect(czp, lx, y, lzp, type));
            newVal = std::max(0, best - opacity);
        }
        int oldVal = chunk->getSkyLight({ lx, y, lz });
        if (oldVal == newVal) return;
        chunk->setSkyLight({ lx, y, lz }, uint8_t(newVal));
        if (world.onBlockUpdate) world.onBlockUpdate(
            PendingBlock{
                .block{ BlockType(blockId), chunk->getMeta({ lx, y, lz }) },
                .block_pos{ x, y, z },
                .light{ chunk->getBlockLight({ lx, y, lz }), chunk->getSkyLight({ lx, y, lz }) }
            }, chunk->cpos);
    }
    else {
        int emitted = Blocks::blockProperties[blockId].lightEmission;
        if (opacity < 15) {
            int best = 0;
            best = std::max(best, getLightDirect(cxn, lxn, y, lz, type));
            best = std::max(best, getLightDirect(cxp, lxp, y, lz, type));
            best = std::max(best, getLightDirect(chunk, lx, y - 1, lz, type));
            best = std::max(best, getLightDirect(chunk, lx, y + 1, lz, type));
            best = std::max(best, getLightDirect(czn, lx, y, lzn, type));
            best = std::max(best, getLightDirect(czp, lx, y, lzp, type));
            newVal = std::max(emitted, best - opacity);
        }
        else {
            newVal = emitted;
        }
        int oldVal = chunk->getBlockLight({ lx, y, lz });
        if (oldVal == newVal) return;
        chunk->setBlockLight({ lx, y, lz }, uint8_t(newVal));
        if (world.onBlockUpdate) world.onBlockUpdate(
            PendingBlock{
                .block{ BlockType(blockId), chunk->getMeta({ lx, y, lz }) },
                .block_pos{ x, y, z },
                .light{ chunk->getBlockLight({ lx, y, lz }), chunk->getSkyLight({ lx, y, lz }) }
            }, chunk->cpos);
    }

    // Fan out — push directly, bypass merge (BFS fan-out must not be merged).
    auto maybeQueue = [&](int nx, int ny, int nz, Chunk* nc, int nlx, int nlz) {
        if (ny < 0 || ny >= CHUNK_HEIGHT || !nc) return;
        int expected = std::max(0, newVal - 1);
        if (getLightDirect(nc, nlx, ny, nlz, type) < expected && lightQueue.size() < 1000000)
            lightQueue.push_back({ {nx, ny, nz}, {nx, ny, nz}, type });
        };

    maybeQueue(x - 1, y, z, cxn, lxn, lz);
    maybeQueue(x + 1, y, z, cxp, lxp, lz);
    maybeQueue(x, y - 1, z, chunk, lx, lz);
    maybeQueue(x, y + 1, z, chunk, lx, lz);
    maybeQueue(x, y, z - 1, czn, lx, lzn);
    maybeQueue(x, y, z + 1, czp, lx, lzp);
}

void Lighter::unlightAt(int x, int y, int z, LightType type, WorldManager& world) {
    if (y < 0 || y >= CHUNK_HEIGHT) return;

    ChunkCache cache;
    cache.refresh(x >> 4, z >> 4, world);
    Chunk* chunk = cache.grid[1][1];
    if (!chunk) return;

    int lx = x & 15, lz = z & 15;
    int oldVal = (type == LightType::Sky)
        ? chunk->getSkyLight({ lx, y, lz })
        : chunk->getBlockLight({ lx, y, lz });
    if (oldVal == 0) return;

    if (type == LightType::Sky) chunk->setSkyLight({ lx, y, lz }, 0);
    else                        chunk->setBlockLight({ lx, y, lz }, 0);

    unlightQueue.push_back({ {x, y, z}, type, oldVal });

    while (!unlightQueue.empty()) {
        auto [pos, t, val] = unlightQueue.back();
        unlightQueue.pop_back();

        cache.refresh(pos.x >> 4, pos.z >> 4, world);

        const int ndx[] = { -1, 1,  0, 0, 0, 0 };
        const int ndy[] = { 0, 0, -1, 1, 0, 0 };
        const int ndz[] = { 0, 0,  0, 0,-1, 1 };
        for (int i = 0; i < 6; ++i) {
            int nx = pos.x + ndx[i];
            int ny = pos.y + ndy[i];
            int nz = pos.z + ndz[i];
            if (ny < 0 || ny >= CHUNK_HEIGHT) continue;

            int ncx = nx >> 4, ncz = nz >> 4;
            int nlx = nx & 15, nlz = nz & 15;
            int dx = ncx - cache.cx, dz = ncz - cache.cz;
            Chunk* nc = (dx >= -1 && dx <= 1 && dz >= -1 && dz <= 1)
                ? cache.grid[dx + 1][dz + 1]
                : world.getChunkRaw({ ncx, ncz });
            if (!nc) continue;

            int nVal = (t == LightType::Sky)
                ? nc->getSkyLight({ nlx, ny, nlz })
                : nc->getBlockLight({ nlx, ny, nlz });
            if (nVal == 0) continue;

            if (nVal < val) {
                if (t == LightType::Sky) nc->setSkyLight({ nlx, ny, nlz }, 0);
                else                     nc->setBlockLight({ nlx, ny, nlz }, 0);
                unlightQueue.push_back({ {nx, ny, nz}, t, nVal });
            }
            else {
                scheduleLightUpdate({ nx, ny, nz }, t);
            }
        }
    }
}

bool Lighter::processLightQueue(WorldManager& world) {
    if (processingDepth >= 50) return !lightQueue.empty();
    ++processingDepth;

    ChunkCache cache;

    while (!lightQueue.empty()) {
        LightRegion region = lightQueue.back();
        lightQueue.pop_back();

        int dx = region.max.x - region.min.x + 1;
        int dy = region.max.y - region.min.y + 1;
        int dz = region.max.z - region.min.z + 1;
        if (dx * dy * dz > 32768) continue;

        region.min.y = std::max(region.min.y, 0);
        region.max.y = std::min(region.max.y, CHUNK_HEIGHT - 1);

        for (int x = region.min.x; x <= region.max.x; ++x)
            for (int z = region.min.z; z <= region.max.z; ++z)
                for (int y = region.min.y; y <= region.max.y; ++y)
                    propagateLightAt(x, y, z, region.type, world, cache);
    }

    --processingDepth;
    return false;
}