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

// Get colliders for an area
std::vector<AABB> WorldManager::getCollidingBoundingBoxes(const AABB& area) {
    std::vector<AABB> collidingBoxes;

    int minX = Java::DoubleToInt32(std::floor(area.minX));
    int maxX = Java::DoubleToInt32(std::floor(area.maxX + 1.0));
    int minY = Java::DoubleToInt32(std::floor(area.minY));
    int maxY = Java::DoubleToInt32(std::floor(area.maxY + 1.0));
    int minZ = Java::DoubleToInt32(std::floor(area.minZ));
    int maxZ = Java::DoubleToInt32(std::floor(area.maxZ + 1.0));

    // Java iterates Y from var5-1 to var6 (exclusive)
    int startY = CrossPlatform::Math::max(0, minY - 1);
    int endY = CrossPlatform::Math::min(127, maxY);

    // Iterate for our potential grid
    for (int x = minX; x < maxX; x++) {
        for (int z = minZ; z < maxZ; z++) {
            // Get the chunk once for this X/Z column
            Chunk* chunk = getChunkRaw({ x >> 4, z >> 4 });

            // If chunk isn't loaded, Beta 1.7.3 usually treats it as air 
            if (!chunk) continue;

            // local coords inside the chunk
            int localX = x & 15;
            int localZ = z & 15;

            for (int y = startY; y <= endY; ++y) {
                BlockType block_id = chunk->getBlock({ localX, y, localZ });
                // Air isn't collidable
                if (block_id == BlockType::BLOCK_AIR) continue;
                if (!Blocks::blockProperties[block_id].isCollidable) continue;
                uint8_t block_meta = chunk->getMeta({ localX, y, localZ });
                // Offset local collider to world coordinates
                CollisionShape worldCollider = Blocks::blockBehaviors[block_id].getCollider(block_meta).offset(x, y, z);
                for (auto& box : worldCollider.boxes)
                    if (box.intersects(area))
                        collidingBoxes.push_back(box);
            }
        }
    }
    return collidingBoxes;
}

// Tick
void WorldManager::tick(const std::vector<ClientPosition>& players) {
    elapsed_ticks++;
    drainGenQueue();       // process generation results first
    updateLoadRadius(players);
    populateReady();       // population runs on main thread
    lightManager.processLightQueue(*this, INT_MAX);
}

void WorldManager::update(const std::vector<ClientPosition>& players) {
    pumpPipeline(players);
}

void WorldManager::seedChunkLighting(Int32_2 pos) {
    auto* chunk = getChunkRaw(pos);
    if (!chunk) return;

    // We check each column in the chunk's height against its neighbors, if they differ then we schedule light updates for the vertical column between them.
    // This works like 99% of the time but can miss some edge cases; its fast though!
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
                int minY = CrossPlatform::Math::min(thisH, neighborH);
                int maxY = CrossPlatform::Math::max(thisH, neighborH);
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
	propagateChunkLightBorders(pos);
}

void WorldManager::updateLoadRadius(const std::vector<ClientPosition>& players) {
	// Get all the chunk positions we want to be loaded based on player positions
    std::unordered_set<Int32_2> wanted;
    for (const auto& player : players) {
        Int2 center = player.getChunkPos();
        for (int dx = -VIEW_RADIUS; dx <= VIEW_RADIUS; dx++)
            for (int dz = -VIEW_RADIUS; dz <= VIEW_RADIUS; dz++)
                wanted.insert({ center.x + dx, center.z + dz });
    }

    // Make dummy chunks for any new positions we want 
	// The generater picks up that this chunk is not generated yet and fills it in, then replaces the chunk in the map with the real one when done
    for (const auto& pos : wanted) {
        if (!chunks.contains(pos)) {
            auto c = std::make_shared<Chunk>();
            c->cpos = pos;
            chunks.emplace(pos, std::move(c));
        }
    }

    // Remove chunks we don't want
    for (auto it = chunks.begin(); it != chunks.end(); ) {
        if (wanted.contains(it->first)) { ++it; continue; }
        if (it->second->spawnChunk) { ++it; continue; }
        ChunkState s = it->second->state.load();
        if (s == ChunkState::Generating) {
            // Leave the slot so drainGenQueue can still find it
            ++it;
            continue;
        }
        it = chunks.erase(it);
    }
}

void WorldManager::pumpPipeline(const std::vector<ClientPosition>& players) {
    // Take a snapshot of all the current chunk positions so we don't have to worry about threads
    // This is technically a relic from when we had chunks put themselves into the world's chunk map but now the world does it all at the end of the tick
    // Still good practice, though
    std::vector<Int32_2> snapshot;
    snapshot.reserve(chunks.size());
    for (auto& [pos, chunk] : chunks)
        snapshot.push_back(pos);

    const int playerCount = int(players.size());
    const int MAX_GENERATIONS_PER_TICK = 4; // per player
    const int slicePerPlayer = (playerCount > 0)
        ? CrossPlatform::Math::max(1, MAX_GENERATIONS_PER_TICK / playerCount)
        : MAX_GENERATIONS_PER_TICK;

    std::vector<Int32_2> noPlayerCandidates;
    std::vector<std::vector<Int32_2>> perPlayerQueues;
    if (playerCount == 0) {
        // No players so try and get every chunk within load distance if its not already generating
        for (const Int32_2& p : snapshot) {
            auto it = chunks.find(p);
            if (it == chunks.end()) continue;
            if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded) continue;
            noPlayerCandidates.push_back(p);
        }
        std::sort(noPlayerCandidates.begin(), noPlayerCandidates.end(), [](const Int32_2& a, const Int32_2& b) {
            if (a.x != b.x) return a.x < b.x;
            return a.z < b.z;
            });
    }
    else {
        perPlayerQueues.reserve(size_t(playerCount));
        for (const auto& player : players) {
            std::vector<Int32_2> candidates;
            candidates.reserve(snapshot.size());
            for (const Int32_2& p : snapshot) {
                auto it = chunks.find(p);
                if (it == chunks.end()) continue;
                if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded) continue;
                candidates.push_back(p);
            }
            // Sort by load order that beta 1.7.3 seems to use
            std::sort(candidates.begin(), candidates.end(), [](const Int32_2& a, const Int32_2& b) {
                if (a.x != b.x) return a.x < b.x;
                return a.z < b.z;
                });
            perPlayerQueues.push_back(std::move(candidates));
        }
    }

    std::unordered_set<Int32_2> startedThisTick;
    auto startGeneration = [&](const Int32_2& pos) -> bool {
		// Check if already started this tick (can happen with multiple players), and if chunk is still Unloaded (can be changed by another thread).
        if (startedThisTick.contains(pos)) return false;
        auto it = chunks.find(pos);
        if (it == chunks.end()) return false;
        if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded) return false;

        // Actually generate this chunk
        it->second->state.store(ChunkState::Generating, std::memory_order_release);
        pool.detach_task([pos, this]() {
            // We make a new chunk here instead of modifying the existing chunk because multithreading is a pain
            // The placeholder chunk in the map will be replaced by this one when we push to genDoneQueue
            auto chunk = std::make_shared<Chunk>();
            chunk->cpos = pos;
            thread_local Generator tl_gen(this->seed);
            tl_gen.GenerateChunk(*chunk);
            chunk->isModified = true;
            chunk->generateSkylightMap();
            chunk->state.store(ChunkState::Generated, std::memory_order_release);

            // This just posts the result so we can start lighting and check for population
            this->postGenResult(std::move(chunk));
            });
        startedThisTick.insert(pos);
        return true;
        };

    if (playerCount == 0) {
        // No players so no budget slicing!
        int started = 0;
        for (const Int32_2& pos : noPlayerCandidates) {
            if (started >= MAX_GENERATIONS_PER_TICK) break;
            if (startGeneration(pos)) ++started;
        }
    }
    else {
        // We have players so we try and make sure everyone gets an equal number of chunks each tick
        // We don't want a player to 
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

void WorldManager::populateReady() {
    // Try and match beta's population order its finicky lol
    std::vector<Int32_2> ordered;
    ordered.reserve(chunks.size());
    for (auto& [pos, chunk] : chunks) {
        if (chunk->isTerrainPopulated) continue;
        // Only consider chunks that could possibly be ready
        // This excludes border chunks on the positive X and Z axes since their population needs neighbors that can't exist
        if (!chunks.contains({ pos.x + 1, pos.z }) ||
            !chunks.contains({ pos.x, pos.z + 1 }) ||
            !chunks.contains({ pos.x + 1, pos.z + 1 }))
            continue;
        ordered.push_back(pos);
    }

	// Sort by population order that beta 1.7.3 seems to use
    std::sort(ordered.begin(), ordered.end(), [](const Int32_2& a, const Int32_2& b) {
        if (a.x != b.x) return a.x < b.x;
        return a.z < b.z;
        });

	// Make sure we don't try to populate the same chunk multiple times in one tick (can happen with the weird population order and multiple players)
    // Also make sure we populate in the right order!
    // We break if the target chunk isn't ready yet so population order is guaranteed
    std::unordered_set<Int32_2> populatedThisTick;
    for (const Int32_2& pos : ordered) {
        if (!canPopulateDirect(pos)) break;
        if (populatedThisTick.contains(pos)) continue;
        auto cit = chunks.find(pos);
        if (cit == chunks.end()) break;
        cit->second->state.store(ChunkState::Populating, std::memory_order_release);
        thread_local Generator tl_gen(this->seed);
        tl_gen.PopulateChunk(*cit->second, *this);
        cit->second->isTerrainPopulated = true;
        cit->second->isModified = true;
        cit->second->state.store(ChunkState::Populated, std::memory_order_release);
        populatedThisTick.insert(pos);
    }
}