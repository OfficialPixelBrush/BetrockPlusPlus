/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "world.h"
#include "generator/chunk_gen.h"

void WorldManager::tick(const std::vector<ClientPosition>& players) {
    elapsed_ticks++;
    updateLoadRadius(players);
    pumpPipeline();
}

void WorldManager::updateLoadRadius(const std::vector<ClientPosition>& players) {
    std::unordered_set<ChunkPos> wanted;
    for (const auto& player : players) {
        Int2 center = player.getChunkPos();
        for (int dx = -VIEW_RADIUS; dx <= VIEW_RADIUS; dx++)
            for (int dz = -VIEW_RADIUS; dz <= VIEW_RADIUS; dz++)
                wanted.insert({ center.x + dx, center.z + dz });
    }

    std::lock_guard lock(chunksMutex);

    // Allocate new chunks
    for (const auto& pos : wanted) {
        if (!chunks.contains(pos)) {
            auto c = std::make_unique<Chunk>();
            c->cpos = pos;
            chunks.emplace(pos, std::move(c));
        }
    }

    // Unload any chunk not wanted by any player
    for (auto it = chunks.begin(); it != chunks.end(); ) {
        if (wanted.contains(it->first)) {
            ++it;
            continue;
        }
        ChunkState s = it->second->state.load();
        if (s == ChunkState::Generating || s == ChunkState::Poulating || s == ChunkState::Lighting) {
            ++it; // worker still owns this chunk, can't erase yet
            continue;
        }
        it = chunks.erase(it);
    }
}

void WorldManager::pumpPipeline() {
    std::vector<ChunkPos> snapshot;
    {
        std::lock_guard lock(chunksMutex);
        snapshot.reserve(chunks.size());
        for (auto& [pos, chunk] : chunks)
            snapshot.push_back(pos);
    }

    for (const ChunkPos& pos : snapshot) {
        // Generate terrain for Unloaded chunks
        {
            std::lock_guard lock(chunksMutex);
            auto it = chunks.find(pos);
            if (it != chunks.end() &&
                it->second->state.load(std::memory_order_acquire) == ChunkState::Unloaded) {
                it->second->state.store(ChunkState::Generating, std::memory_order_release);
                std::shared_ptr<Chunk> chunkRef = it->second;
                pool.detach_task([chunkRef, this]() {
                    thread_local Generator tl_gen(this->seed);
                    tl_gen.GenerateChunk(*chunkRef);
                    chunkRef->generateSkylightMap();
                    chunkRef->isModified = true;
                    {
                        std::lock_guard lock(chunksMutex);
                        flushPendingBlocks(chunkRef->cpos);
                    }
                    chunkRef->state.store(ChunkState::Generated, std::memory_order_release);
                    });
                continue;
            }
        }

        // Check if this chunk completing generation unlocks population
        // for itself or any of its -x, -z, -x-z neighbors
        {
            std::lock_guard lock(chunksMutex);
            for (ChunkPos candidate : {
                pos,
                    ChunkPos{ pos.x - 1, pos.z },
                    ChunkPos{ pos.x,   pos.z - 1 },
                    ChunkPos{ pos.x - 1, pos.z - 1 }
            }) {
                if (!canPopulateLocked(candidate)) continue;
                auto it = chunks.find(candidate);
                if (it == chunks.end()) continue;
                it->second->state.store(ChunkState::Poulating, std::memory_order_release);
                std::shared_ptr<Chunk> chunkRef = it->second;
                pool.detach_task([chunkRef, this]() {
                    thread_local Generator tl_gen(this->seed);
                    tl_gen.PopulateChunk(*chunkRef, *this);
                    chunkRef->isTerrainPopulated = true;
                    chunkRef->isModified = true;
                    chunkRef->state.store(ChunkState::Populated, std::memory_order_release);
                    });
                break; // one population task per snapshot entry
            }
        }
    }
}