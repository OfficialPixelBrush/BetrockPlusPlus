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

struct WorldManager {
    std::unordered_map<ChunkPos, std::shared_ptr<Chunk>> chunks;
    std::mutex chunksMutex;

    BS::thread_pool<> pool{ std::max(1u, uint32_t(float(std::thread::hardware_concurrency()) * 0.25f)) };

    int64_t seed = 0;

    void tick(const std::vector<ClientPosition>& players);

    Chunk* getChunk(ChunkPos pos) {
        std::lock_guard lock(chunksMutex);
        auto it = chunks.find(pos);
        return (it != chunks.end()) ? it->second.get() : nullptr;
    }

    int getViewRadius() {
        return VIEW_RADIUS;
    }

    int getSimulationDistance() {
        return SIMULATION_RADIUS;
    }

private:
    static constexpr int VIEW_RADIUS = 15;
    static constexpr int SIMULATION_RADIUS = 9;

    void updateLoadRadius(const std::vector<ClientPosition>& players);
    void pumpPipeline();
};