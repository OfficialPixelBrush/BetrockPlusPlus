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
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <deque>
#include <thread>
#include <atomic>

struct PendingBlock {
    Block block{ BLOCK_AIR, 0 };
    Int3 block_pos{ 0, 0, 0 };
};

struct WorldManager {
    std::unordered_map<ChunkPos, std::shared_ptr<Chunk>> chunks;
    std::unordered_map<ChunkPos, std::vector<PendingBlock>> pending_blocks;
    std::mutex chunksMutex;

    BS::thread_pool<> pool{ std::max(1u, uint32_t(float(std::thread::hardware_concurrency()) * 0.25f)) };

    int64_t seed = 0;
    int64_t elapsed_ticks = 0;

    WorldManager() {}

    ~WorldManager() {}

    void tick(const std::vector<ClientPosition>& players);
    int getViewRadius() { return VIEW_RADIUS; }
    int getSimulationDistance() { return SIMULATION_RADIUS; }

    // Public chunk accessor (acquires lock)
    std::shared_ptr<Chunk> getChunk(ChunkPos pos) {
        std::lock_guard lock(chunksMutex);
        return getChunkLocked(pos);
    }

    bool canPopulate(ChunkPos pos) {
        std::lock_guard lock(chunksMutex);
        return canPopulateLocked(pos);
    }

    BlockType getBlockId(Int3 wpos) {
        std::lock_guard lock(chunksMutex);
        auto* chunk = getChunkRaw({ wpos.x >> 4, wpos.z >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated)
            return BlockType::BLOCK_AIR;
        return chunk->getBlock({ wpos.x & 15, wpos.y, wpos.z & 15 });
    }

    uint8_t getMetadata(Int3 wpos) {
        std::lock_guard lock(chunksMutex);
        auto* chunk = getChunkRaw({ wpos.x >> 4, wpos.z >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getMeta({ wpos.x & 15, wpos.y, wpos.z & 15 });
    }

    void setBlock(Int3 wpos, BlockType block_type, uint8_t metadata = 0) {
        std::lock_guard lock(chunksMutex);
        ChunkPos cp{ wpos.x >> 4, wpos.z >> 4 };
        auto* chunk = getChunkRaw(cp);
        if (!chunk || chunk->state.load() < ChunkState::Generated) {
            pending_blocks[cp].push_back({
                .block = Block{.type = block_type, .data = metadata },
                .block_pos = { wpos.x & 15, wpos.y, wpos.z & 15 }
                });
            return;
        }
        Int3 local{ wpos.x & 15, wpos.y, wpos.z & 15 };
        chunk->setBlock(local, block_type);
        chunk->setMeta(local, metadata);
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
        std::lock_guard lock(chunksMutex);
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
        std::lock_guard lock(chunksMutex);
        auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getHeightValue({ wx & 15, wz & 15 });
    }

    // Must be called with chunksMutex held.
    void flushPendingBlocks(ChunkPos pos) {
        auto pit = pending_blocks.find(pos);
        if (pit == pending_blocks.end()) return;
        auto* chunk = getChunkRaw(pos);
        if (!chunk) return;
        for (auto& pb : pit->second) {
            chunk->setBlock(pb.block_pos, pb.block.type);
            chunk->setMeta(pb.block_pos, pb.block.data);
        }
        pending_blocks.erase(pit);
    }

private:
    static constexpr int VIEW_RADIUS = 15;
    static constexpr int SIMULATION_RADIUS = 9;

    void updateLoadRadius(const std::vector<ClientPosition>& players);
    void pumpPipeline(const std::vector<ClientPosition>& players);

    Chunk* getChunkRaw(ChunkPos pos) {
        auto it = chunks.find(pos);
        return (it != chunks.end()) ? it->second.get() : nullptr;
    }

    std::shared_ptr<Chunk> getChunkLocked(ChunkPos pos) {
        auto it = chunks.find(pos);
        return (it != chunks.end()) ? it->second : nullptr;
    }

    bool canPopulateLocked(ChunkPos pos) {
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