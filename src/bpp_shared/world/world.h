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
#include "world/chunk.h"
#include "world/client_pos.h"
#include "BS_thread_pool.hpp"
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include <vector>

struct PendingBlock {
    BlockType block_type = BlockType::BLOCK_AIR;
    uint8_t metadata = 0;
    Int3 block_pos{ 0, 0, 0 };
};

struct WorldManager {
    std::unordered_map<ChunkPos, std::shared_ptr<Chunk>> chunks;
    std::unordered_map<ChunkPos, std::vector<PendingBlock>> pending_blocks;
    std::mutex chunksMutex;

    BS::thread_pool<> pool{ std::max(1u, uint32_t(float(std::thread::hardware_concurrency()) * 0.25f)) };

    int64_t seed = 0;
    int64_t elapsed_ticks = 0;

    void tick(const std::vector<ClientPosition>& players);
    int getViewRadius() { return VIEW_RADIUS; }
    int getSimulationDistance() { return SIMULATION_RADIUS; }

    // Public chunk accessor (acquires lock)
    std::shared_ptr<Chunk> getChunk(ChunkPos pos) {
        std::lock_guard lock(chunksMutex);
        return getChunkLocked(pos);
    }

    // Checks whether chunk and its +x, +z, +x+z neighbors are all Generated.
    bool canPopulate(ChunkPos pos) {
        std::lock_guard lock(chunksMutex);
        return canPopulateLocked(pos);
    }

    // Returns air for out-of-bounds or ungenerated chunks.
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

    // Writes the block. If the target chunk is not yet generated, queues
    // the write as a pending block to be flushed when the chunk finishes.
    void setBlock(Int3 wpos, BlockType block_type, uint8_t metadata = 0) {
        // TO-DO: this needs to cause lighting updates!!
        std::lock_guard lock(chunksMutex);
        ChunkPos cp{ wpos.x >> 4, wpos.z >> 4 };
        auto* chunk = getChunkRaw(cp);
        if (!chunk || chunk->state.load() < ChunkState::Generated) {
            pending_blocks[cp].push_back({
                .block_type = block_type,
                .metadata = metadata,
                .block_pos = { wpos.x & 15, wpos.y, wpos.z & 15 }
                });
            return;
        }
        Int3 local{ wpos.x & 15, wpos.y, wpos.z & 15 };
        chunk->setBlock(local, block_type);
        chunk->setMeta(local, metadata);
    }

    // Returns true if the block at wpos is air (or chunk not loaded).
    bool isAirBlock(Int3 wpos) {
        return getBlockId(wpos) == BlockType::BLOCK_AIR;
    }

    // Returns the material of the block at wpos. Returns Material::air if unloaded.
    Material getMaterial(Int3 wpos) {
        return Blocks::blockProperties[getBlockId(wpos)].material;
    }

    // Returns true if the block is a full, opaque, normal cube.
    bool isBlockNormalCube(Int3 wpos) {
        BlockType block = getBlockId(wpos);
        if (block == BlockType::BLOCK_AIR) return false;
        const auto& props = Blocks::blockProperties[block];
        return props.material.isSolid && props.isNormalCube;
    }

    // Returns the Y of the highest solid-or-liquid block + 1.
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

    // Returns the height map value for the given world XZ.
    // This is the Y of the highest non-air block + 1.
    int getHeightValue(int wx, int wz) {
        std::lock_guard lock(chunksMutex);
        auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getHeightValue({ wx & 15, wz & 15 });
    }

    // Flushes any pending blocks into the chunk now that it's generated.
    // Call this right after a chunk transitions to Generated, before lighting.
    // Must be called with chunksMutex held.
    void flushPendingBlocks(ChunkPos pos) {
        auto pit = pending_blocks.find(pos);
        if (pit == pending_blocks.end()) return;
        auto* chunk = getChunkRaw(pos);
        if (!chunk) return;
        for (auto& pb : pit->second) {
            chunk->setBlock(pb.block_pos, pb.block_type);
            chunk->setMeta(pb.block_pos, pb.metadata);
        }
        pending_blocks.erase(pit);
    }

private:
    static constexpr int VIEW_RADIUS = 15;
    static constexpr int SIMULATION_RADIUS = 9;

    void updateLoadRadius(const std::vector<ClientPosition>& players);
    void pumpPipeline();

    // Internal helpers (must be called with chunksMutex already held)

    // Raw pointer version
    Chunk* getChunkRaw(ChunkPos pos) {
        auto it = chunks.find(pos);
        return (it != chunks.end()) ? it->second.get() : nullptr;
    }

    // shared_ptr version for use when the caller needs to keep the chunk alive.
    std::shared_ptr<Chunk> getChunkLocked(ChunkPos pos) {
        auto it = chunks.find(pos);
        return (it != chunks.end()) ? it->second : nullptr;
    }

    // Population check; assumes chunksMutex is held.
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