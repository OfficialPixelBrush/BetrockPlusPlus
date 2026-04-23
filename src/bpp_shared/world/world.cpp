/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "world.h"
#include "generator/chunk_gen.h"
#include "debug_generator/debug_generator.h"

// Tick
void WorldManager::tick(const std::vector<ClientPosition>& players) {
    elapsed_ticks++;
    updateLoadRadius(players);
    pumpPipeline(players);
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

    for (const auto& pos : wanted) {
        if (!chunks.contains(pos)) {
            auto c = std::make_unique<Chunk>();
            c->cpos = pos;
            chunks.emplace(pos, std::move(c));
        }
    }

    for (auto it = chunks.begin(); it != chunks.end(); ) {
        if (wanted.contains(it->first)) { ++it; continue; }
        ChunkState s = it->second->state.load();
        if (s == ChunkState::Generating || s == ChunkState::Poulating || s == ChunkState::Lighting) {
            ++it;
            continue;
        }
        it = chunks.erase(it);
    }
}

void WorldManager::pumpPipeline(const std::vector<ClientPosition>& players) {
    // Snapshot all chunk positions while holding the lock.
    std::vector<ChunkPos> snapshot;
    {
        std::lock_guard lock(chunksMutex);
        snapshot.reserve(chunks.size());
        for (auto& [pos, chunk] : chunks)
            snapshot.push_back(pos);
    }

    // Total budget is fixed at MAX_GENERATIONS_PER_TICK.
    constexpr int MAX_GENERATIONS_PER_TICK = 8;
    const int playerCount = static_cast<int>(players.size());
    const int slicePerPlayer = (playerCount > 0)
        ? std::max(1, MAX_GENERATIONS_PER_TICK / playerCount)
        : MAX_GENERATIONS_PER_TICK;

    // Build a sorted candidate list for each player
    std::vector<std::vector<ChunkPos>> perPlayerQueues;
    perPlayerQueues.reserve(size_t(playerCount));

    for (const auto& player : players) {
        Int2 centre = player.getChunkPos();
        std::vector<ChunkPos> candidates;
        candidates.reserve(snapshot.size());

        {
            std::lock_guard lock(chunksMutex);
            for (const ChunkPos& p : snapshot) {
                auto it = chunks.find(p);
                if (it == chunks.end()) continue;
                if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded) continue;
                candidates.push_back(p);
            }
        }

        std::sort(candidates.begin(), candidates.end(), [&](const ChunkPos& a, const ChunkPos& b) {
            int da = std::max(std::abs(a.x - centre.x), std::abs(a.z - centre.z));
            int db = std::max(std::abs(b.x - centre.x), std::abs(b.z - centre.z));
            return da < db;
            });

        perPlayerQueues.push_back(std::move(candidates));
    }

    // Fallback for zero-player case: sort by distance from origin.
    std::vector<ChunkPos> noPlayerCandidates;
    if (playerCount == 0) {
        {
            std::lock_guard lock(chunksMutex);
            for (const ChunkPos& p : snapshot) {
                auto it = chunks.find(p);
                if (it == chunks.end()) continue;
                if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded) continue;
                noPlayerCandidates.push_back(p);
            }
        }
        std::sort(noPlayerCandidates.begin(), noPlayerCandidates.end(), [](const ChunkPos& a, const ChunkPos& b) {
            return (a.x * a.x + a.z * a.z) < (b.x * b.x + b.z * b.z);
            });
    }

    // Kick off generation jobs, respecting each player's slice.
    std::unordered_set<ChunkPos> startedThisTick;

    auto startGeneration = [&](const ChunkPos& pos) -> bool {
        if (startedThisTick.contains(pos)) return false;
        std::lock_guard lock(chunksMutex);
        auto it = chunks.find(pos);
        if (it == chunks.end()) return false;
        if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded) return false;
        it->second->state.store(ChunkState::Generating, std::memory_order_release);
        std::shared_ptr<Chunk> chunkRef = it->second;
        pool.detach_task([chunkRef, this]() {
            thread_local Generator tl_gen(this->seed);
            tl_gen.GenerateChunk(*chunkRef);
            //DebugGenerator::generateChunk(*chunkRef);
            chunkRef->generateSkylightMap();
            chunkRef->isModified = true;
            {
                std::lock_guard lock(chunksMutex);
                flushPendingBlocks(chunkRef->cpos);
            }
            chunkRef->state.store(ChunkState::Generated, std::memory_order_release);
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
                int playerStarted [[maybe_unused]] = static_cast<int>(
                    std::count_if(startedThisTick.begin(), startedThisTick.end(),
                        [&](const ChunkPos&) { return true; }) // placeholder; tracked below
                    );
                // Count how many this player has consumed via their own cursor progress.
                // Simpler: just give each player up to slicePerPlayer from their sorted list.
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

    for (const ChunkPos& pos : snapshot) {
        // Population
        {
            std::lock_guard lock(chunksMutex);
            auto it = chunks.find(pos);
            if (it != chunks.end() &&
                (it->second->state.load(std::memory_order_acquire) == ChunkState::Generated)) {
                for (ChunkPos candidate : {
                    pos,
                        ChunkPos{ pos.x - 1, pos.z },
                        ChunkPos{ pos.x,     pos.z - 1 },
                        ChunkPos{ pos.x - 1, pos.z - 1 }
                })
                {
                    if (!canPopulateLocked(candidate)) continue;
                    auto cit = chunks.find(candidate);
                    if (cit == chunks.end()) continue;
                    cit->second->state.store(ChunkState::Poulating, std::memory_order_release);
                    std::shared_ptr<Chunk> chunkRef = cit->second;
                    pool.detach_task([chunkRef, this]() {
                        thread_local Generator tl_gen(this->seed);
                        tl_gen.PopulateChunk(*chunkRef, *this);
                        chunkRef->isTerrainPopulated = true;
                        chunkRef->isModified = true;
                        chunkRef->state.store(ChunkState::Populated, std::memory_order_release);
                        });
                    break;
                }
            }
        }
    }
}