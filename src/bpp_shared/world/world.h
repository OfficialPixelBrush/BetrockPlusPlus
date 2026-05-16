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
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include <vector>
#include <deque>
#include <atomic>
#include <algorithm>
#include "base_structs.h"
#include "blocks.h"
#include "helpers/AABB.h"
#include "java_math.h"
#include "world/chunk.h"
#include "world/client_pos.h"
#include "BS_thread_pool.hpp"
#include "lighter.h"
#include "tile_entities/tile_entity_manager.h"
#include "storage/region.h"

struct PendingBlock {
    Block block{ BLOCK_AIR, 0 };
    Int3 block_pos{ 0, 0, 0 };
    Int2 light{ 0, 15 }; // block light, sky light
};

struct WorldManager {
    std::unordered_map<Int32_2, std::shared_ptr<Chunk>> chunks;
    std::function<void(PendingBlock, Int32_2)> onBlockUpdate;

    std::unordered_map<Int32_2, std::vector<std::pair<Int3, Block>>> pendingBleedWrites;

    std::mutex genDoneMutex;
    std::deque<std::shared_ptr<Chunk>> genDoneQueue;

    Lighter lightManager;
    TileEntityManager tileEntityManager;

    BS::thread_pool<> pool{ 2 };
    BS::thread_pool<> populationPool{ 1 };

    int64_t seed = 0;
    int64_t elapsed_ticks = 0;

    Int3 spawnPoint{ 0, 0, 0 };

    Java::Random rand;

    WorldManager(bool isHell = false): isHell(isHell) {}

    ~WorldManager() {}

    void initWorldSeed(std::string pSeed){
		this->seed = hashCode(pSeed);
    }

    void initWorldSeed(int64_t pSeed){
		this->seed = pSeed;
    }
    void tick(const std::vector<ClientPosition>& players);
    void update(const std::vector<ClientPosition>& players);
    std::vector<AABB> getCollidingBoundingBoxes(const AABB& area);
    int getViewRadius() { return VIEW_RADIUS; }
    int getSimulationDistance() { return SIMULATION_RADIUS; }
    void updateLoadRadius(const std::vector<ClientPosition>& players);
    void pumpPipeline(const std::vector<ClientPosition>& players);
    void populateReady();

    void createTileEntity(std::shared_ptr<TileEntity> tileEntity) {
        Int32_2 Int32_2 = { tileEntity->position.x >> 4, tileEntity->position.z >> 4 };
        Chunk* chunk = getChunkRaw(Int32_2);
        if (!chunk) return;
        tileEntityManager.initializeTileEntity(tileEntity); // weak_ptr added if canTick
        chunk->tileEntities.push_back(std::move(tileEntity)); // chunk takes ownership
    }

    // Returns the tile entity at world position `pos`, or nullptr if none.
    TileEntity* getTileEntity(Int3 pos) {
        Chunk* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
        if (!chunk) return nullptr;
        for (auto& te : chunk->tileEntities) {
            if (te && te->position.x == pos.x &&
                te->position.y == pos.y &&
                te->position.z == pos.z)
                return te.get();
        }
        return nullptr;
    }

    // Typed lookup — returns nullptr if not found or wrong type.
    template<typename T>
    T* getTileEntityAs(Int3 pos) {
        return dynamic_cast<T*>(getTileEntity(pos));
    }

    template<typename T>
    std::shared_ptr<T> getTileEntityShared(Int3 pos) {
        Chunk* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
        if (!chunk) return nullptr;
        for (auto& te : chunk->tileEntities) {
            if (te && te->position.x == pos.x &&
                te->position.y == pos.y &&
                te->position.z == pos.z)
                return std::dynamic_pointer_cast<T>(te);
        }
        return nullptr;
    }

    // Remove the tile entity at world position `pos`.
    void removeTileEntity(Int3 pos) {
        Chunk* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
        if (!chunk) return;
        auto& tes = chunk->tileEntities;
        tes.erase(std::remove_if(tes.begin(), tes.end(), [&](const std::shared_ptr<TileEntity>& te) {
            return te && te->position.x == pos.x &&
                te->position.y == pos.y &&
                te->position.z == pos.z;
            }), tes.end());
    }

    // Called from pool gen threads
    void postGenResult(std::shared_ptr<Chunk> chunk) {
        std::lock_guard lk(genDoneMutex);
        genDoneQueue.push_back(std::move(chunk));
    }

    void drainGenQueue() {
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

    std::shared_ptr<Chunk> getChunk(Int32_2 pos) {
        return getChunkShared(pos);
    }

    bool canPopulate(Int32_2 pos) {
        return canPopulateDirect(pos);
    }

    BlockType getBlockId(Int3 wpos) {
        if (!inBounds(wpos.y)) return BlockType::BLOCK_AIR;
        auto* chunk = getChunkRaw({ wpos.x >> 4, wpos.z >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated)
            return BlockType::BLOCK_AIR;
        return chunk->getBlock({ wpos.x & 15, wpos.y, wpos.z & 15 });
    }

    uint8_t getMetadata(Int3 wpos) {
        if (!inBounds(wpos.y)) return 0;
        auto* chunk = getChunkRaw({ wpos.x >> 4, wpos.z >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getMeta({ wpos.x & 15, wpos.y, wpos.z & 15 });
    }

    void setBlock(Int3 wpos, BlockType block_type, uint8_t metadata = 0) {
        if (!inBounds(wpos.y)) return;
        Int32_2 cp{ wpos.x >> 4, wpos.z >> 4 };
        auto* chunk = getChunkRaw(cp);
        if (!chunk || chunk->state.load() < ChunkState::Generated || chunk->inUse.load()) {
            // Target chunk isn't ready; cache the write for replay
            pendingBleedWrites[cp].push_back({ wpos, Block{ block_type, metadata } });
            return;
        }

        // Remove any tile entities that exist at this spot
        auto& tes = chunk->tileEntities;
        tes.erase(std::remove_if(tes.begin(), tes.end(), [&](const std::shared_ptr<TileEntity>& te) {
            return te && te->position == wpos;
            }), tes.end());

        // Unlight before changing the block
        lightManager.unlightAt(wpos.x, wpos.y, wpos.z, LightType::Block, *this);
        lightManager.unlightAt(wpos.x, wpos.y, wpos.z, LightType::Sky, *this);

        // Get the local coordinates of this block within the chunk and set it
        int lx = wpos.x & 15;
        int lz = wpos.z & 15;
        Int3 local{ lx, wpos.y, lz };
        chunk->setBlock(local, block_type);
        chunk->setMeta(local, metadata);

        int y = wpos.y; int x = wpos.x; int z = wpos.z;
        int oldHeight = chunk->getHeightValue({ lx, lz });

        if (Blocks::blockProperties[block_type].lightOpacity != 0) {
            // Placing opaque block; heightmap may rise
            if (y >= oldHeight) {
                chunk->relightColumn({ lx, lz });

                // The column below the new top was zeroed out by relightColumn.
                // Notify the BFS that all blocks from y down to oldHeight need updating
                for (int sy = oldHeight; sy <= y; ++sy)
                    lightManager.unlightAt(x, sy, z, LightType::Sky, *this);
            }
        }
        else if (y == oldHeight - 1) {
            // Removing top opaque block; heightmap may fall
            chunk->relightColumn({ lx, lz });
        }

        int newHeight = chunk->getHeightValue({ lx, lz });
        if (newHeight < oldHeight) {
            for (int sy = newHeight; sy < oldHeight; ++sy)
                lightManager.scheduleLightUpdate({ x, sy, z }, LightType::Sky);
        }

        // Always re-evaluate the edited block and its 4 horizontal neighbours
        // across the height transition band.
        lightManager.scheduleLightUpdate({ x, y, z }, LightType::Sky);
        const int ndx[] = { -1, 1,  0, 0 };
        const int ndz[] = { 0, 0, -1, 1 };
        for (int i = 0; i < 4; ++i) {
            int nx = x + ndx[i], nz = z + ndz[i];
            int neighborHeight = getHeightValue(nx, nz);
            int thisHeight = chunk->getHeightValue({ lx, lz });
            if (neighborHeight == thisHeight) continue;
            int minY = CrossPlatform::Math::min(thisHeight, neighborHeight);
            int maxY = CrossPlatform::Math::max(thisHeight, neighborHeight);
            lightManager.scheduleLightRegion({ nx, minY, nz }, { nx, maxY, nz }, LightType::Sky);
        }
        // Schedule a block light update for the position itself
        lightManager.scheduleLightUpdate({ x, y, z }, LightType::Block);

        // Callback for the client and server to know about this block update
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

    void initSpawn() {
        int sx = 0;
        int sz = 0;
        auto canCoordinateBeSpawn = [&](int x, int z) -> bool {
            auto b = getFirstUncoveredBlock(x, z);
            if (b == BlockType::BLOCK_INVALID) {
                // Force generate this chunk so we can check the block type.
                auto cpos = Int32_2{ x >> 4, z >> 4 };
				auto chunk = std::make_shared<Chunk>();
				chunk->cpos = cpos;
                chunk->spawnChunk = true;
                chunks[cpos] = std::move(chunk);
				// Wait until this chunk is done generating. We don't care about populating or lighting
				while (chunks[cpos]->state.load() < ChunkState::Generated) {
					this->pumpPipeline({});
                    this->pool.wait();
                    this->drainGenQueue();
                };
                b = getFirstUncoveredBlock(x, z);
            }
            return getFirstUncoveredBlock(x, z) == BlockType::BLOCK_SAND;
            };
        for (; !canCoordinateBeSpawn(sx, sz); sz += this->rand.nextInt(64) - this->rand.nextInt(64)) {
            sx += this->rand.nextInt(64) - this->rand.nextInt(64);
        }
        this->spawnPoint = { sx, 64, sz };
		chunks.clear(); // Clear all chunks so we can start fresh from the spawn area
    }

    Int3 getSpawnPoint(bool adjust) {
        if (!adjust) return spawnPoint;
        int sx = this->spawnPoint.x;
        int sz = this->spawnPoint.z;
		sx += rand.nextInt(20) - 10;
		sz += rand.nextInt(20) - 10;
		int sy = findTopSolidBlock(sx, sz);
		return { sx, sy, sz };
    }

    BlockType getFirstUncoveredBlock(int wx, int wz) {
        auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return BlockType(-1);
        int lx = wx & 15, lz = wz & 15;
        int y = 63;
        while (y < 127 && chunk->getBlock({ lx, y + 1, lz }) != BlockType::BLOCK_AIR) {
            ++y;
        }
        return chunk->getBlock({ lx, y, lz });
    }

    int getHeightValue(int wx, int wz) {
        auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getHeightValue({ wx & 15, wz & 15 });
    }

    // Returns the baked temperature/humidity for a world column.
    double getTemperatureAt(int wx, int wz) {
        auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0.5;
        return double(chunk->getTemperature({ wx & 15, wz & 15 }));
    }

    double getHumidityAt(int wx, int wz) {
        auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0.5;
        return double(chunk->getHumidity({ wx & 15, wz & 15 }));
    }

    int getSkyLight(Int3 pos) {
        if (!inBounds(pos.y)) return 0;
        auto* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getSkyLight({ pos.x & 15, pos.y, pos.z & 15 });
    }

    int getBlockLight(Int3 pos) {
        if (!inBounds(pos.y)) return 0;
        auto* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getBlockLight({ pos.x & 15, pos.y, pos.z & 15 });
    }

    void propagateChunkLightBorders(Int32_2 cpos) {
        // Iterate through our chunk borders
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
                if (ndx[i] == -1) { lx = 0; lz = t; nx = 15; nz = t; }
                else if (ndx[i] == 1) { lx = 15; lz = t; nx = 0; nz = t; }
                else if (ndz[i] == -1) { lx = t; lz = 0; nx = t; nz = 15; }
                else { lx = t; lz = 15; nx = t; nz = 0; }

                for (int y = 0; y < CHUNK_HEIGHT; ++y) {
					// Does our neighbor block have a block light > 0 or sky light > 0? If so, schedule a light update for the block on our side of the border.
                    if (neighborChunk->getBlockLight({ nx, y, nz })) {
                        lightManager.scheduleLightUpdate({ bx + lx, y, bz + lz }, LightType::Block);
                    }
                    if (neighborChunk->getSkyLight({ nx, y, nz }) > 0) {
                        lightManager.scheduleLightUpdate({ bx + lx, y, bz + lz }, LightType::Sky);
                    }
                }
            }
        }
    }

    Chunk* getChunkRaw(Int32_2 pos) {
        auto it = chunks.find(pos);
        return (it != chunks.end()) ? it->second.get() : nullptr;
    }

    void flushBleedWrites() {
        for (auto it = pendingBleedWrites.begin(); it != pendingBleedWrites.end(); ) {
            auto* target = getChunkRaw(it->first);
            if (target && target->state.load() >= ChunkState::Generated && !target->inUse.load()) {
                for (auto& [wpos, block] : it->second)
                    setBlock(wpos, block.type, block.data);
                it = pendingBleedWrites.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    // Returns true when the world-space Y is within valid chunk bounds.
    static constexpr bool inBounds(int y) { return y >= 0 && y < CHUNK_HEIGHT; }

private:
    // I believe the vanilla default is 
    static constexpr int VIEW_RADIUS = 13; // no pixel THIS is the vanilla default :anger:
    static constexpr int SIMULATION_RADIUS = 9;

    bool isHell = false; // for the nether

    void seedChunkLighting(Int32_2 pos);

    std::shared_ptr<Chunk> getChunkShared(Int32_2 pos) {
        auto it = chunks.find(pos);
        return (it != chunks.end()) ? it->second : nullptr;
    }

    // Check if a chunk can be populated
    bool canPopulateDirect(Int32_2 pos) {
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