/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "world.h"
#include "blocks.h"
#include "generator/overworld/chunk_gen.h"
#include "generator/nether/chunk_gen.h"
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
    drainLoadQueue();      // integrate finished loads

    // Queue any modified chunks for saving
    if (regionManager) {
        for (auto& [pos, chunk] : chunks) {
            if (!chunk->isModified) continue;
            ChunkState s = chunk->state.load();
            if (s < ChunkState::Generated) continue;
            if (s == ChunkState::Generating || s == ChunkState::Loading) continue;
            regionManager->saveChunk(chunk);
            chunk->isModified = false;
        }
    }

    if (regionManager) regionManager->pumpPipeline();
    updateLoadRadius(players);
    populateReady();       // population runs on main thread
    lightManager.processLightQueue(*this, INT_MAX);

    // Update our entities
    entityManager.tick();
}

void WorldManager::update(const std::vector<ClientPosition>& players) {
    pumpPipeline(players);
}

void WorldManager::shutdown() {
    if (!regionManager) return;
    if (isHell) {
        GlobalLogger().info << "Saving chunks for level -1\n";
    }
    else {
        GlobalLogger().info << "Saving chunks for level 0\n";
    }

    // Save all currently loaded modified chunks
    for (auto& [pos, chunk] : chunks) {
        if (!chunk->isModified && !chunk->tileEntities.size()) continue; // unconditionally save chunks with tile entities for now this is a bandaid fix!!
        ChunkState s = chunk->state.load();
        if (s < ChunkState::Generated) continue;
        if (s == ChunkState::Generating || s == ChunkState::Loading) continue;
        regionManager->saveChunk(chunk);
        chunk->isModified = false;
    }

    // For every position that still has pending bleed writes, force-load or
    // force-generate the chunk, apply the writes, then save it.
    while (!pendingBleedWrites.empty()) {
        auto it = pendingBleedWrites.begin();
        Int32_2 cpos = it->first;

        // Insert a placeholder if not already in the map
        if (!chunks.contains(cpos)) {
            auto c = std::make_shared<Chunk>();
            c->cpos = cpos;
            chunks.emplace(cpos, std::move(c));
        }

        // Spin until this chunk is ready — load from disk or generate fresh
        while (chunks[cpos]->state.load() < ChunkState::Generated) {
            pumpPipeline({});
            pool.wait();
            drainGenQueue();
            regionManager->iopool.wait();
            drainLoadQueue();
        }

        // Apply the pending writes
        flushBleedWrites();

        // Save it
        auto& chunk = chunks[cpos];
        if (chunk->isModified) {
            regionManager->saveChunk(chunk);
            chunk->isModified = false;
        }
    }

    // Flush everything to disk and wait for IO to finish
    regionManager->flushAll();
}

void WorldManager::drainGenQueue() {
    // Integrate chunks that finished generating
    std::deque<std::shared_ptr<Chunk>> ready;
    {
        std::lock_guard lk(genDoneMutex);
        ready.swap(genDoneQueue);
    }
    for (auto& c : ready) {
        Int32_2 pos = c->cpos;
        auto it = chunks.find(pos);
        if (it != chunks.end()) {
            bool wasSpawnChunk = it->second->spawnChunk;
            it->second = std::move(c);
            it->second->spawnChunk = wasSpawnChunk;

            // Replay any writes that arrived while this chunk was unloaded.
            auto pit = pendingBleedWrites.find(pos);
            if (pit != pendingBleedWrites.end()) {
                for (auto& [wpos, block] : pit->second)
                    setBlock(wpos, block.type, block.data);
                pendingBleedWrites.erase(pit);
            }
            seedChunkLighting(pos);
        }
    }
}

void WorldManager::drainLoadQueue() {
    for (auto& [pos, chunk] : chunks) {
        if (chunk->state.load(std::memory_order_acquire) != ChunkState::Loading) continue;
        auto loaded = regionManager->getChunk(pos);
        if (!loaded) continue;

        auto it = chunks.find(pos);
        if (it == chunks.end()) continue;
        bool wasSpawnChunk = it->second->spawnChunk;
        it->second = std::move(loaded);
        it->second->spawnChunk = wasSpawnChunk;

        // Regenerate temp and humidity data
        thread_local BiomeGenerator tl_biomeGen(0);
        thread_local bool tl_biomeGenInit = false;
        if (!tl_biomeGenInit) {
            tl_biomeGen = BiomeGenerator(this->seed);
            tl_biomeGenInit = true;
        }
        std::vector<double> temp, humi, weird;
        Biome ignored[CHUNK_AREA];
        tl_biomeGen.GenerateBiomeMap(
            ignored, temp, humi, weird,
            Int2{ pos.x * CHUNK_WIDTH, pos.z * CHUNK_WIDTH }
        );
        for (int i = 0; i < CHUNK_AREA; ++i) {
            it->second->temperature[i] = float(temp[i]);
            it->second->humidity[i] = float(humi[i]);
        }

        // Replay any writes that arrived while this chunk was loading.
        auto pit = pendingBleedWrites.find(pos);
        if (pit != pendingBleedWrites.end()) {
            for (auto& [wpos, block] : pit->second)
                setBlock(wpos, block.type, block.data);
            pendingBleedWrites.erase(pit);
        }
        //seedChunkLighting(pos);
    }
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
        int viewDist = (player.viewDistanceOverride) ? player.viewDistanceOverride : VIEW_RADIUS;
        for (int dx = -viewDist; dx <= viewDist; dx++)
            for (int dz = -viewDist; dz <= viewDist; dz++)
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
        if (s == ChunkState::Generating || s == ChunkState::Loading) {
            // Leave the slot so drainGenQueue/drainLoadQueue can still find it
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
    const int slicePerPlayer = 16;

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

    auto startLoading = [&](const Int32_2& pos) -> bool {
        if (startedThisTick.contains(pos)) return false;
        auto it = chunks.find(pos);
        if (it == chunks.end()) return false;
        if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded) return false;
        it->second->state.store(ChunkState::Loading, std::memory_order_release);
        regionManager->loadChunk(pos);
        startedThisTick.insert(pos);
        return true;
        };

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
            if (isHell) {
                thread_local NetherGenerator tl_gen(this->seed);
                tl_gen.GenerateChunk(*chunk);
            }
            else {
                thread_local OverworldGenerator tl_gen(this->seed);
                tl_gen.GenerateChunk(*chunk);
            }
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
        int started = 0;
        for (const Int32_2& pos : noPlayerCandidates) {
            if (started >= slicePerPlayer) break;
            if (regionManager->chunkExists(pos)) {
                if (startLoading(pos)) ++started;
                continue;
            }
            if (startGeneration(pos)) ++started;
        }
    }
    else {
        // Make sure everyone gets their share of the budget
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
                    Int32_2 cpos = perPlayerQueues[size_t(i)][size_t(cur)];
                    ++cur;
                    if (regionManager->chunkExists(cpos)) {
                        if (startLoading(cpos)) {
                            ++playerConsumed;
                            ++totalStarted;
                            anyProgress = true;
                        }
                        continue;
                    }
                    if (startGeneration(cpos)) {
                        ++playerConsumed;
                        ++totalStarted;
                        anyProgress = true;
                    }
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
        WorldWrapper wrapper{ .m_manager = *this, .m_centerChunkPos = pos };
        wrapper.m_centerChunkPos = pos;
        wrapper.getChunkRegion();
        if (isHell) {
            NetherGenerator tl_gen(this->seed);
            tl_gen.PopulateChunk(*cit->second, wrapper);
        }
        else {
            OverworldGenerator tl_gen(this->seed);
            tl_gen.PopulateChunk(*cit->second, wrapper);
        }
        cit->second->isTerrainPopulated = true;
        cit->second->isModified = true;
        cit->second->state.store(ChunkState::Populated, std::memory_order_release);
        populatedThisTick.insert(pos);
        wrapper.freeChunkRegion();
        flushBleedWrites();
    }
}