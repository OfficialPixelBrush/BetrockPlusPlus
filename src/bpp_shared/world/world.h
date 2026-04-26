/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

// The world manager acts like a wrapper around the chunk manager and lighting manager.
// It handles all world-related operations and provides a simple interface for the rest of the code to interact with the world.
// WorldManager.h
#pragma once
#include "base_structs.h"
#include "blocks.h"
#include "world/chunk.h"
#include "world/client_pos.h"
#include "BS_thread_pool.hpp"
#include "lighter.h"
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include <vector>
#include <deque>
#include <atomic>

struct PendingBlock {
    Block block{ BLOCK_AIR, 0 };
    Int3 block_pos{ 0, 0, 0 };
    Int2 light{ 0, 15 }; // block light, sky light
};

struct WorldManager {
    // Owned exclusively by the main thread — no locks needed for any read/write.
    std::unordered_map<ChunkPos, std::shared_ptr<Chunk>> chunks;

    std::function<void(PendingBlock, ChunkPos)> onBlockUpdate; // always called on main thread

    // Tiny queue: pool gen threads post finished chunks here; main thread drains
    // each tick. Only this deque needs a mutex — the chunks map itself does not.
    std::mutex genDoneMutex;
    std::deque<std::shared_ptr<Chunk>> genDoneQueue;

    Lighter lightManager;

    BS::thread_pool<> pool{ 2 };

    int64_t seed = 0;
    int64_t elapsed_ticks = 0;

    WorldManager() {}

    ~WorldManager() {}

    void tick(const std::vector<ClientPosition>& players);
    void update(const std::vector<ClientPosition>& players);
    int getViewRadius() { return VIEW_RADIUS; }
    int getSimulationDistance() { return SIMULATION_RADIUS; }

    // Called from pool gen threads
    void postGenResult(std::shared_ptr<Chunk> chunk) {
        std::lock_guard lk(genDoneMutex);
        genDoneQueue.push_back(std::move(chunk));
    }

    // Called from main thread at the top of tick() — integrates finished chunks.
    void drainGenQueue() {
        std::deque<std::shared_ptr<Chunk>> ready;
        {
            std::lock_guard lk(genDoneMutex);
            ready.swap(genDoneQueue);
        }
        for (auto& c : ready) {
            ChunkPos pos = c->cpos;
            auto it = chunks.find(pos);
            if (it != chunks.end()) {
                it->second = std::move(c);
                seedChunkLighting(pos);
            }
        }
    }

    // All accessors below are main-thread only
    std::shared_ptr<Chunk> getChunk(ChunkPos pos) {
        return getChunkShared(pos);
    }

    bool canPopulate(ChunkPos pos) {
        return canPopulateDirect(pos);
    }

    BlockType getBlockId(Int3 wpos) {
        auto* chunk = getChunkRaw({ wpos.x >> 4, wpos.z >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated)
            return BlockType::BLOCK_AIR;
        return chunk->getBlock({ wpos.x & 15, wpos.y, wpos.z & 15 });
    }

    uint8_t getMetadata(Int3 wpos) {
        auto* chunk = getChunkRaw({ wpos.x >> 4, wpos.z >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getMeta({ wpos.x & 15, wpos.y, wpos.z & 15 });
    }

    void setBlock(Int3 wpos, BlockType block_type, uint8_t metadata = 0) {
        ChunkPos cp{ wpos.x >> 4, wpos.z >> 4 };
        auto* chunk = getChunkRaw(cp);
        if (!chunk || chunk->state.load() < ChunkState::Generated) return;

        // Unlight before changing the block
        lightManager.unlightAt(wpos.x, wpos.y, wpos.z, LightType::Block, *this);
        lightManager.unlightAt(wpos.x, wpos.y, wpos.z, LightType::Sky, *this);

        Int3 local{ wpos.x & 15, wpos.y, wpos.z & 15 };
        chunk->setBlock(local, block_type);
        chunk->setMeta(local, metadata);

        int lx = wpos.x & 15;
        int lz = wpos.z & 15;
        int y = wpos.y; int x = wpos.x; int z = wpos.z;

        int oldHeight = chunk->getHeightValue({ lx, lz });
        if (Blocks::blockProperties[block_type].lightOpacity != 0) {
            // placing opaque block; heightmap may rise
            if (y >= oldHeight)
                chunk->relightColumn({ lx, lz });
        }
        else if (y == oldHeight - 1) {
            // removing top opaque block; heightmap may fall
            chunk->relightColumn({ lx, lz });
        }
        int newHeight = chunk->getHeightValue({ lx, lz });

        // directly set sky light for newly exposed column (height fell)
        if (newHeight < oldHeight) {
            for (int sy = newHeight; sy < oldHeight; ++sy) {
                chunk->setSkyLight({ lx, sy, lz }, 15);
            }
        }

        // schedule sky relaxation for this position and its 4 horizontal neighbors
        lightManager.scheduleLightUpdate({ x, y, z }, LightType::Sky);
        const int ndx[] = { -1, 1,  0, 0 };
        const int ndz[] = { 0, 0, -1, 1 };
        for (int i = 0; i < 4; ++i) {
            int nx = x + ndx[i], nz = z + ndz[i];
            int neighborHeight = getHeightValue(nx, nz);
            int thisHeight2 = chunk->getHeightValue({ lx, lz });
            if (neighborHeight == thisHeight2) continue;
            int minY = std::min(thisHeight2, neighborHeight);
            int maxY = std::max(thisHeight2, neighborHeight);
            for (int sy = minY; sy <= maxY; ++sy)
                lightManager.scheduleLightUpdate({ nx, sy, nz }, LightType::Sky);
        }

        lightManager.scheduleLightUpdate({ x, y, z }, LightType::Block);

        if (onBlockUpdate) onBlockUpdate(
            PendingBlock{
                .block{block_type, metadata},
                .block_pos{wpos.x, wpos.y, wpos.z},
                .light{chunk->getBlockLight(local), chunk->getSkyLight(local)}
            }, chunk->cpos);
    }

    bool isAirBlock(Int3 wpos) {
        return getBlockId(wpos) == BlockType::BLOCK_AIR;
    }

    Material getMaterial(Int3 wpos) {
        return Blocks::blockProperties[getBlockId(wpos)].material;
    }

    bool isBlockNormalCube(Int3 wpos) {
        BlockType block = getBlockId(wpos);
        if (block == BlockType::BLOCK_AIR) return false;
        const auto& props = Blocks::blockProperties[block];
        return props.material.isSolid && props.isNormalCube;
    }

    int findTopSolidBlock(int wx, int wz) {
        auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return -1;
        int lx = wx & 15, lz = wz & 15;
        for (int y = 127; y > 0; --y) {
            BlockType block = chunk->getBlock({ lx, y, lz });
            if (block == BlockType::BLOCK_AIR) continue;
            Material mat = Blocks::blockProperties[block].material;
            if (mat.isSolid || mat.isLiquid) return y + 1;
        }
        return -1;
    }

    int getHeightValue(int wx, int wz) {
        auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getHeightValue({ wx & 15, wz & 15 });
    }

    int getSkyLight(Int3 pos) {
        auto* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getSkyLight({ pos.x & 15, pos.y, pos.z & 15 });
    }

    int getBlockLight(Int3 pos) {
        auto* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getBlockLight({ pos.x & 15, pos.y, pos.z & 15 });
    }

    void propagateChunkSkylightBorders(ChunkPos cpos) {
        Chunk* chunk = getChunkRaw(cpos);
        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                int wx = cpos.x * 16 + x;
                int wz = cpos.z * 16 + z;
                int thisHeight = chunk->getHeightValue({ x, z });

                // check all 4 cardinal neighbors in world space
                const int ndx[] = { -1, 1, 0, 0 };
                const int ndz[] = { 0, 0,-1, 1 };
                for (int i = 0; i < 4; ++i) {
                    int nx = wx + ndx[i];
                    int nz = wz + ndz[i];
                    Chunk* nc = getChunkRaw({ nx >> 4, nz >> 4 });
                    if (!nc) continue;
                    int neighborHeight = nc->getHeightValue({ nx & 15, nz & 15 });
                    // schedule update for the Y range between the two heights
                    int minY = std::min(thisHeight, neighborHeight);
                    int maxY = std::max(thisHeight, neighborHeight);
                    for (int y = minY; y <= maxY; ++y)
                        lightManager.scheduleLightUpdate({ nx, y, nz }, LightType::Sky);
                }
            }
        }
    }

    void propagateChunkBlockLightBorders(ChunkPos cpos) {
        const int ndx[] = { -1, 1,  0, 0 };
        const int ndz[] = { 0, 0, -1, 1 };

        int bx = cpos.x * 16;
        int bz = cpos.z * 16;

        for (int i = 0; i < 4; ++i) {
            Chunk* neighborChunk = getChunkRaw({ cpos.x + ndx[i], cpos.z + ndz[i] });
            if (!neighborChunk) continue;

            // Walk the border edge of this chunk that faces the neighbor
            for (int t = 0; t < 16; ++t) {
                // Pick the border column of this chunk facing direction i
                int lx, lz, nx, nz;
                if (ndx[i] == -1) { lx = 0; lz = t; nx = 16 - 1; nz = t; }
                else if (ndx[i] == 1) { lx = 16 - 1; lz = t; nx = 0; nz = t; }
                else if (ndz[i] == -1) { lx = t; lz = 0; nx = 0; nz = 16 - 1; }
                else { lx = t; lz = 16 - 1; nx = t; nz = 0; }

                for (int y = 0; y < CHUNK_HEIGHT; ++y)
                    // Does our neighbor block have a block light > 0?
                    if (neighborChunk->getBlockLight({ nx, y, nz }))
                        lightManager.scheduleLightUpdate({ bx + lx, y, bz + lz }, LightType::Block);
            }
        }
    }

private:
    static constexpr int VIEW_RADIUS = 15;
    static constexpr int SIMULATION_RADIUS = 9;

    void updateLoadRadius(const std::vector<ClientPosition>& players);
    void pumpPipeline(const std::vector<ClientPosition>& players);
    void populateReady();
    void seedChunkLighting(ChunkPos pos);

    Chunk* getChunkRaw(ChunkPos pos) {
        auto it = chunks.find(pos);
        return (it != chunks.end()) ? it->second.get() : nullptr;
    }

    std::shared_ptr<Chunk> getChunkShared(ChunkPos pos) {
        auto it = chunks.find(pos);
        return (it != chunks.end()) ? it->second : nullptr;
    }

    bool canPopulateDirect(ChunkPos pos) {
        auto* chunk = getChunkRaw(pos);
        if (!chunk) return false;
        if (chunk->isTerrainPopulated) return false;
        if (chunk->state.load() < ChunkState::Generated) return false;
        auto* a = getChunkRaw({ pos.x + 1, pos.z });
        auto* b = getChunkRaw({ pos.x,     pos.z + 1 });
        auto* c = getChunkRaw({ pos.x + 1, pos.z + 1 });
        if (!a || !b || !c) return false;
        if (a->state.load() < ChunkState::Generated) return false;
        if (b->state.load() < ChunkState::Generated) return false;
        if (c->state.load() < ChunkState::Generated) return false;
        return true;
    }
};