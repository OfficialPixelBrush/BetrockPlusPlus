/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "lighter.h"
#include "world.h"

void Lighter::propagateLightAt(int x, int y, int z, LightType type, WorldManager& world) {
    if (y < 0 || y >= CHUNK_HEIGHT) return;

    auto chunk = world.getChunk({ x >> 4, z >> 4 });
    if (!chunk) return;

    int lx = x & 15, lz = z & 15;
    uint8_t blockId = chunk->getBlock({ lx, y, lz });
    int opacity = Blocks::blockProperties[blockId].lightOpacity;
    if (opacity == 0) opacity = 1;

    int newVal = 0;

    if (type == LightType::Sky) {
        if (chunk->canBlockSeeSky({ lx, y, lz })) {
            newVal = 15;
        }
        else if (opacity < 15) {
            int best = 0;
            best = std::max(best, world.getSkyLight({x - 1, y, z}));
            best = std::max(best, world.getSkyLight({x + 1, y, z}));
            best = std::max(best, world.getSkyLight({x, y - 1, z}));
            best = std::max(best, world.getSkyLight({x, y + 1, z}));
            best = std::max(best, world.getSkyLight({x, y, z - 1}));
            best = std::max(best, world.getSkyLight({x, y, z + 1}));
            newVal = std::max(0, best - opacity);
        }
        int oldVal = chunk->getSkyLight({ lx, y, lz });
        if (oldVal == newVal) return;
        chunk->setSkyLight({ lx, y, lz }, (uint8_t)newVal);
        if (world.onBlockUpdate) world.onBlockUpdate(
            PendingBlock{
                .block{world.getBlockId({x, y, z}), world.getMetadata({x, y, z})},
                .block_pos{x, y, z},
                .light{chunk->getBlockLight({ lx, y, lz }), chunk->getSkyLight({ lx, y, lz })}
            }, chunk->cpos);
    }
    else {
        int emitted = Blocks::blockProperties[blockId].lightEmission;
        if (opacity < 15) {
            int best = 0;
            best = std::max(best, world.getBlockLight({x - 1, y, z}));
            best = std::max(best, world.getBlockLight({x + 1, y, z}));
            best = std::max(best, world.getBlockLight({x, y - 1, z}));
            best = std::max(best, world.getBlockLight({x, y + 1, z}));
            best = std::max(best, world.getBlockLight({x, y, z - 1}));
            best = std::max(best, world.getBlockLight({x, y, z + 1}));
            newVal = std::max(emitted, best - opacity);
        }
        else {
            newVal = emitted;
        }
        int oldVal = chunk->getBlockLight({ lx, y, lz });
        if (oldVal == newVal) return;
        chunk->setBlockLight({ lx, y, lz }, (uint8_t)newVal);
        if (world.onBlockUpdate) world.onBlockUpdate(
            PendingBlock{
                .block{world.getBlockId({x, y, z}), world.getMetadata({x, y, z})},
                .block_pos{x, y, z},
                .light{chunk->getBlockLight({ lx, y, lz }), chunk->getSkyLight({ lx, y, lz })}
            }, chunk->cpos);
    }

    auto maybeQueue = [&](int nx, int ny, int nz) {
        if (ny < 0 || ny >= CHUNK_HEIGHT) return;
        int expected = std::max(0, newVal - 1);
        int cur = (type == LightType::Sky)
            ? world.getSkyLight({ nx, ny, nz })
            : world.getBlockLight({ nx, ny, nz });
        if (cur < expected)  // only queue if dimmer than it could be
            lightQueue.push_back({ {nx, ny, nz}, type });
        };
    maybeQueue(x - 1, y, z);
    maybeQueue(x + 1, y, z);
    maybeQueue(x, y - 1, z);
    maybeQueue(x, y + 1, z);
    maybeQueue(x, y, z - 1);
    maybeQueue(x, y, z + 1);
}

void Lighter::unlightAt(int x, int y, int z, LightType type, WorldManager& world) {
    if (y < 0 || y >= CHUNK_HEIGHT) return;
    auto chunk = world.getChunk({ x >> 4, z >> 4 });
    if (!chunk) return;

    int lx = x & 15, lz = z & 15;
    int oldVal = (type == LightType::Sky)
        ? chunk->getSkyLight({ lx, y, lz })
        : chunk->getBlockLight({ lx, y, lz });

    if (oldVal == 0) return;

    // Zero this block
    if (type == LightType::Sky)
        chunk->setSkyLight({ lx, y, lz }, 0);
    else
        chunk->setBlockLight({ lx, y, lz }, 0);

    // BFS unlight — propagate darkness outward
    unlightQueue.push_back({ {x, y, z}, type, oldVal });

    int safety = 1000000;
    while (!unlightQueue.empty()) {
        if (--safety <= 0) { unlightQueue.clear(); break; }
        auto [pos, t, val] = unlightQueue.front();
        unlightQueue.pop_front();

        const int ndx[] = { -1, 1,  0, 0, 0, 0 };
        const int ndy[] = { 0, 0, -1, 1, 0, 0 };
        const int ndz[] = { 0, 0,  0, 0,-1, 1 };
        for (int i = 0; i < 6; ++i) {
            int nx = pos.x + ndx[i];
            int ny = pos.y + ndy[i];
            int nz = pos.z + ndz[i];
            if (ny < 0 || ny >= CHUNK_HEIGHT) continue;

            auto nchunk = world.getChunk({ nx >> 4, nz >> 4 });
            if (!nchunk) continue;
            int nlx = nx & 15, nlz = nz & 15;

            int nVal = (t == LightType::Sky)
                ? nchunk->getSkyLight({ nlx, ny, nlz })
                : nchunk->getBlockLight({ nlx, ny, nlz });

            if (nVal == 0) continue;

            if (nVal < val) {
                // This neighbor was lit by us — zero it and keep unlighting
                if (t == LightType::Sky)
                    nchunk->setSkyLight({ nlx, ny, nlz }, 0);
                else
                    nchunk->setBlockLight({ nlx, ny, nlz }, 0);
                unlightQueue.push_back({ {nx, ny, nz}, t, nVal });
            }
            else {
                // Independent source — seed relight from here
                lightQueue.push_back({ {nx, ny, nz}, t });
            }
        }
    }
}
void Lighter::processLightQueue(WorldManager& world) {
    int safety = 1000000;
    while (!lightQueue.empty()) {
        --safety;
        if (safety <= 0) {
            size_t sz = lightQueue.size();
            lightQueue.clear();
            printf("Light queue safety triggered! size=%zu\n", sz);
            return;
        }
        LightUpdate upd = lightQueue.front();
        lightQueue.pop_front();
        propagateLightAt(upd.pos.x, upd.pos.y, upd.pos.z, upd.type, world);
    }
}