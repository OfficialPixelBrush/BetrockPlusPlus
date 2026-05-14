/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once

#include "java_random.h"
#include "world/world.h"
#include "base_structs.h"
#include "blocks/block_properties.h"
#include "constants.h"
#include "helpers/java/java_math.h"

// 3x3 region of chunk pointers, centered on the chunk being populated
struct ChunkPtrRegion {
    std::shared_ptr<Chunk> chunks[3][3];

    std::shared_ptr<Chunk> getChunk(Int2 pos) const {
        if (pos.x < -1 || pos.x > 1 || pos.z < -1 || pos.z > 1)
            return nullptr;
        return chunks[pos.x + 1][pos.z + 1];
    }
};

// Wrapper for world access during chunk population.
// Holds a 3x3 region of chunk pointers centered on the chunk being populated.
// Chunks are marked inUse on acquire and released on free.
struct WorldWrapper {
    WorldManager& manager;
    ChunkPtrRegion chunkRegion;
    Int2 centerChunkPos;

    // Grab the 3x3 region. Any chunk that is already inUse is left as nullptr
    // (writes to it will fall through to the deferred path via the manager).
    void getChunkRegion() {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                int ax = centerChunkPos.x + dx;
                int az = centerChunkPos.z + dz;
                auto c = manager.getChunk({ ax, az });
                if (!c || c->inUse.load()) {
                    chunkRegion.chunks[dx + 1][dz + 1] = nullptr;
                }
                else {
                    chunkRegion.chunks[dx + 1][dz + 1] = c;
                }
            }
        }
        // Mark all successfully acquired chunks as inUse
        for (auto& row : chunkRegion.chunks)
            for (auto& c : row)
                if (c) c->inUse.store(true);
    }

    void freeChunkRegion() {
        for (auto& row : chunkRegion.chunks)
            for (auto& c : row)
                if (c) c->inUse.store(false);
    }

    // Convert a world-space position to a region-local chunk offset (-1..1, -1..1)
    Int2 getRegionChunkPos(Int3 wPos) const {
        return {
            (wPos.x >> 4) - centerChunkPos.x,
            (wPos.z >> 4) - centerChunkPos.z
        };
    }

    int findTopSolidBlock(int wx, int wz) {
        auto chunk = chunkRegion.getChunk(getRegionChunkPos({ wx, 0, wz }));
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
        auto chunk = chunkRegion.getChunk(getRegionChunkPos({ wx, 0, wz }));
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getHeightValue({ wx & 15, wz & 15 });
    }

    double getTemperatureAt(int wx, int wz) {
        auto chunk = chunkRegion.getChunk(getRegionChunkPos({ wx, 0, wz }));
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0.5;
        return double(chunk->getTemperature({ wx & 15, wz & 15 }));
    }

    double getHumidityAt(int wx, int wz) {
        auto chunk = chunkRegion.getChunk(getRegionChunkPos({ wx, 0, wz }));
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0.5;
        return double(chunk->getHumidity({ wx & 15, wz & 15 }));
    }

    BlockType getBlockId(Int3 wpos) const {
        if (!inBounds(wpos.y)) return BlockType::BLOCK_AIR;
        auto chunk = chunkRegion.getChunk(getRegionChunkPos(wpos));
        // Falls outside our grabbed region — ask the manager directly (read-only, safe)
        if (!chunk)
            return manager.getBlockId(wpos);
        if (chunk->state.load() < ChunkState::Generated)
            return BlockType::BLOCK_AIR;
        return chunk->getBlock({ wpos.x & 15, wpos.y, wpos.z & 15 });
    }

    void setBlock(Int3 wpos, BlockType type, uint8_t meta = 0) {
        if (!inBounds(wpos.y)) return;
        auto chunk = chunkRegion.getChunk(getRegionChunkPos(wpos));
        if (!chunk || chunk->state.load() < ChunkState::Generated) {
            // Outside our locked region
            manager.setBlock(wpos, type, meta);
            return;
        }
        chunk->setBlock({ wpos.x & 15, wpos.y, wpos.z & 15 }, type);
        chunk->setMeta({ wpos.x & 15, wpos.y, wpos.z & 15 }, meta);
        chunk->relightColumn({ wpos.x & 15, wpos.z & 15 });

        // Callback for the client and server to know about this block update
        if (manager.onBlockUpdate) manager.onBlockUpdate(
            PendingBlock{
                .block{type, meta},
                .block_pos{wpos.x, wpos.y, wpos.z},
                .light{chunk->getBlockLight({ wpos.x & 15, wpos.y, wpos.z & 15 }), chunk->getSkyLight({ wpos.x & 15, wpos.y, wpos.z & 15 })}
            }, chunk->cpos);
    }

    uint8_t getSkyLight(Int3 wpos) const {
        if (!inBounds(wpos.y)) return 0;
        auto chunk = chunkRegion.getChunk(getRegionChunkPos(wpos));
        if (!chunk || chunk->state.load() < ChunkState::Generated) return 0;
        return chunk->getSkyLight({ wpos.x & 15, wpos.y, wpos.z & 15 });
    }

    int64_t getSeed() const { return manager.seed; }

    // Returns true when the world-space Y is within valid chunk bounds.
    static constexpr bool inBounds(int y) { return y >= 0 && y < CHUNK_HEIGHT; }
};

// Inline block-property helpers
inline bool IsSolid(BlockType t) { return Blocks::blockProperties[t].material.isSolid; }
inline bool IsLiquid(BlockType t) { return Blocks::blockProperties[t].material.isLiquid; }
inline bool IsOpaque(BlockType t) { return Blocks::blockProperties[t].lightOpacity > 0; }

// FeatureGenerator — Beta 1.7.3 world-decoration features
class FeatureGenerator {
public:
    BlockType type = BLOCK_AIR;
    int8_t meta = 0;

    FeatureGenerator() {}
    explicit FeatureGenerator(BlockType pType) : type(pType) {}
    FeatureGenerator(BlockType pType, int8_t pMeta) : type(pType), meta(pMeta) {}

    bool GenerateLake(WorldWrapper& world, Java::Random& rand, Int3 pos);
    bool GenerateDungeon(WorldWrapper& world, Java::Random& rand, Int3 pos);
    bool GenerateClay(WorldWrapper& world, Java::Random& rand, Int3 pos, int32_t blobSize = 0);
    bool GenerateMinable(WorldWrapper& world, Java::Random& rand, Int3 pos, int32_t blobSize = 0);
    bool GenerateFlowers(WorldWrapper& world, Java::Random& rand, Int3 pos);
    bool GenerateTallgrass(WorldWrapper& world, Java::Random& rand, Int3 pos);
    bool GenerateDeadbush(WorldWrapper& world, Java::Random& rand, Int3 pos);
    bool GenerateSugarcane(WorldWrapper& world, Java::Random& rand, Int3 pos);
    bool GeneratePumpkins(WorldWrapper& world, Java::Random& rand, Int3 pos);
    bool GenerateCacti(WorldWrapper& world, Java::Random& rand, Int3 pos);
    bool GenerateLiquid(WorldWrapper& world, Java::Random& rand, Int3 pos);

    static int         GenerateDungeonChestLoot(Java::Random& rand);
    static std::string PickMobToSpawn(Java::Random& rand);
};