/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "world.h"
#include "generator/chunk_gen.h"

void WorldManager::tick(const std::vector<ClientPosition>& players) {
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
        if (s == ChunkState::Generating) {
            ++it; // worker still owns this chunk, can't erase yet
            continue;
        }
        it = chunks.erase(it);
    }
}

// 404 challenge seed
// Generator gen(404);

void WorldManager::pumpPipeline() {
    std::vector<ChunkPos> snapshot;
    {
        std::lock_guard lock(chunksMutex);
        snapshot.reserve(chunks.size());
        for (auto& [pos, chunk] : chunks)
            snapshot.push_back(pos);
    }

    for (const ChunkPos& pos : snapshot) {
        std::shared_ptr<Chunk> chunkRef;
        {
            std::lock_guard lock(chunksMutex);
            auto it = chunks.find(pos);
            if (it == chunks.end()) continue;
            if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded)
                continue;
            // Mark Generating while still holding the lock so updateLoadRadius
            // can't erase it between now and when the lambda runs
            it->second->state.store(ChunkState::Generating, std::memory_order_release);
            chunkRef = it->second;  // shared_ptr keeps it alive in the lambda too
        }

        pool.detach_task([chunkRef, this]() {
            thread_local Generator tl_gen(this->seed);
            tl_gen.GenerateChunk(*chunkRef);
            chunkRef->generateSkylightMap();
            chunkRef->isModified = true;
            chunkRef->state.store(ChunkState::Lit, std::memory_order_release);
            });
    }
}