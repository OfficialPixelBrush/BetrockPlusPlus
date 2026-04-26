/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "world.h"
#include "blocks.h"
#include "generator/chunk_gen.h"
#include "debug_generator/debug_generator.h"

// Tick
void WorldManager::tick(const std::vector<ClientPosition>& players) {
    elapsed_ticks++;
    drainGenQueue();       // integrate finished gen results first
    updateLoadRadius(players);
    populateReady();       // population runs on main thread — direct world access

    lightManager.processLightQueue(*this);
}

// Update
void WorldManager::update(const std::vector<ClientPosition>& players) {
    pumpPipeline(players);
}

void WorldManager::seedChunkLighting(ChunkPos pos) {
    auto* chunk = getChunkRaw(pos);
    if (!chunk) return;

    int bx = pos.x * 16;
    int bz = pos.z * 16;

    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            int wx = bx + x, wz = bz + z;
            int thisH = chunk->getHeightValue({ x, z });
            const int ndx[] = { -1, 1,  0, 0 };
            const int ndz[] = { 0, 0, -1, 1 };
            for (int i = 0; i < 4; ++i) {
                int nx = wx + ndx[i], nz = wz + ndz[i];
                int neighborH = getHeightValue(nx, nz);
                if (neighborH == thisH) continue;
                int minY = std::min(thisH, neighborH);
                int maxY = std::max(thisH, neighborH);
                lightManager.scheduleLightRegion({ nx, minY, nz }, { nx, maxY, nz }, LightType::Sky);
            }
        }
    }

    // Block light emitters
    for (int x = 0; x < 16; ++x)
        for (int z = 0; z < 16; ++z)
            for (int y = 0; y < CHUNK_HEIGHT; ++y) {
                BlockType id = chunk->getBlock({ x, y, z });
                if (Blocks::blockProperties[id].lightEmission > 0)
                    lightManager.scheduleLightUpdate({ bx + x, y, bz + z }, LightType::Block);
            }

    // Pull block light in from already-loaded neighbors
    propagateChunkSkylightBorders(pos);
    propagateChunkBlockLightBorders(pos);
}

void WorldManager::updateLoadRadius(const std::vector<ClientPosition>& players) {
    std::unordered_set<ChunkPos> wanted;
    for (const auto& player : players) {
        Int2 center = player.getChunkPos();
        for (int dx = -VIEW_RADIUS; dx <= VIEW_RADIUS; dx++)
            for (int dz = -VIEW_RADIUS; dz <= VIEW_RADIUS; dz++)
                wanted.insert({ center.x + dx, center.z + dz });
    }

    for (const auto& pos : wanted) {
        if (!chunks.contains(pos)) {
            auto c = std::make_shared<Chunk>();
            c->cpos = pos;
            chunks.emplace(pos, std::move(c));
        }
    }

    for (auto it = chunks.begin(); it != chunks.end(); ) {
        if (wanted.contains(it->first)) { ++it; continue; }
        if (it->second->spawnChunk) { ++it; continue; }
        ChunkState s = it->second->state.load();
        if (s == ChunkState::Generating) {
            // Leave the slot so drainGenQueue can still find it; the gen
            // thread holds a shared_ptr so the Chunk won't be freed.
            ++it;
            continue;
        }
        it = chunks.erase(it);
    }
}

void WorldManager::pumpPipeline(const std::vector<ClientPosition>& players) {
    // Snapshot chunk positions (main thread owns map, no lock needed).
    std::vector<ChunkPos> snapshot;
    snapshot.reserve(chunks.size());
    for (auto& [pos, chunk] : chunks)
        snapshot.push_back(pos);

    const int MAX_GENERATIONS_PER_TICK = 8;
    const int playerCount = static_cast<int>(players.size());
    const int slicePerPlayer = (playerCount > 0)
        ? std::max(1, MAX_GENERATIONS_PER_TICK / playerCount)
        : MAX_GENERATIONS_PER_TICK;

    // Build per-player sorted candidate lists.
    std::vector<std::vector<ChunkPos>> perPlayerQueues;
    perPlayerQueues.reserve(size_t(playerCount));

    for (const auto& player : players) {
        Int2 centre = player.getChunkPos();
        std::vector<ChunkPos> candidates;
        candidates.reserve(snapshot.size());
        for (const ChunkPos& p : snapshot) {
            auto it = chunks.find(p);
            if (it == chunks.end()) continue;
            if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded) continue;
            candidates.push_back(p);
        }
        std::sort(candidates.begin(), candidates.end(), [&](const ChunkPos& a, const ChunkPos& b) {
            int da = std::max(std::abs(a.x - centre.x), std::abs(a.z - centre.z));
            int db = std::max(std::abs(b.x - centre.x), std::abs(b.z - centre.z));
            return da < db;
            });
        perPlayerQueues.push_back(std::move(candidates));
    }

    // Fallback: zero-player case sorts by distance from origin.
    std::vector<ChunkPos> noPlayerCandidates;
    if (playerCount == 0) {
        for (const ChunkPos& p : snapshot) {
            auto it = chunks.find(p);
            if (it == chunks.end()) continue;
            if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded) continue;
            noPlayerCandidates.push_back(p);
        }
        std::sort(noPlayerCandidates.begin(), noPlayerCandidates.end(), [](const ChunkPos& a, const ChunkPos& b) {
            return (a.x * a.x + a.z * a.z) < (b.x * b.x + b.z * b.z);
            });
    }

    std::unordered_set<ChunkPos> startedThisTick;

    // Kick off a generation job for a chunk slot.
    // The pool worker owns a fresh Chunk privately; it never touches world.chunks.
    // When done it posts via postGenResult(); main thread integrates in drainGenQueue().
    auto startGeneration = [&](const ChunkPos& pos) -> bool {
        if (startedThisTick.contains(pos)) return false;
        auto it = chunks.find(pos);
        if (it == chunks.end()) return false;
        if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded) return false;
        it->second->state.store(ChunkState::Generating, std::memory_order_release);
        pool.detach_task([pos, this]() {
            auto chunk = std::make_shared<Chunk>();
            chunk->cpos = pos;
            thread_local Generator tl_gen(this->seed);
            tl_gen.GenerateChunk(*chunk);
            chunk->isModified = true;
            chunk->generateSkylightMap();
            chunk->state.store(ChunkState::Generated, std::memory_order_release);
            this->postGenResult(std::move(chunk));
            });
        startedThisTick.insert(pos);
        return true;
        };

    if (playerCount == 0) {
        int started = 0;
        for (const ChunkPos& pos : noPlayerCandidates) {
            if (started >= MAX_GENERATIONS_PER_TICK) break;
            if (startGeneration(pos)) ++started;
        }
    }
    else {
        std::vector<int> cursors(size_t(playerCount), 0);
        int totalStarted = 0;
        const int totalBudget = slicePerPlayer * playerCount;
        bool anyProgress = true;
        while (totalStarted < totalBudget && anyProgress) {
            anyProgress = false;
            for (int i = 0; i < playerCount && totalStarted < totalBudget; ++i) {
                int& cur = cursors[size_t(i)];
                int playerConsumed = 0;
                while (playerConsumed < slicePerPlayer && cur < static_cast<int>(perPlayerQueues[size_t(i)].size())) {
                    if (startGeneration(perPlayerQueues[size_t(i)][size_t(cur)])) {
                        ++playerConsumed;
                        ++totalStarted;
                        anyProgress = true;
                    }
                    ++cur;
                }
            }
        }
    }
}

// Population runs synchronously on the main thread each tick.
// This is safe because main thread owns the chunk map, so neighbour reads
// are direct with no locking needed.
void WorldManager::populateReady() {
    // Snapshot positions to avoid iterator invalidation if populate ever
    // modifies the map (it shouldn't, but be defensive).
    std::vector<ChunkPos> snapshot;
    snapshot.reserve(chunks.size());
    for (auto& [pos, _] : chunks)
        snapshot.push_back(pos);

    std::unordered_set<ChunkPos> populatedThisTick;

    for (const ChunkPos& pos : snapshot) {
        auto it = chunks.find(pos);
        if (it == chunks.end()) continue;
        if (it->second->state.load(std::memory_order_acquire) != ChunkState::Generated) continue;

        for (ChunkPos candidate : {
            pos,
                ChunkPos{ pos.x - 1, pos.z },
                ChunkPos{ pos.x,     pos.z - 1 },
                ChunkPos{ pos.x - 1, pos.z - 1 }
        })
        {
            if (populatedThisTick.contains(candidate)) continue;
            if (!canPopulateDirect(candidate)) continue;
            auto cit = chunks.find(candidate);
            if (cit == chunks.end()) continue;
            // Populate directly on main thread.
            cit->second->state.store(ChunkState::Poulating, std::memory_order_release);
            thread_local Generator tl_gen(this->seed);
            tl_gen.PopulateChunk(*cit->second, *this);
            cit->second->isTerrainPopulated = true;
            cit->second->isModified = true;
            cit->second->state.store(ChunkState::Populated, std::memory_order_release);
            populatedThisTick.insert(candidate);
            break;
        }
    }
}