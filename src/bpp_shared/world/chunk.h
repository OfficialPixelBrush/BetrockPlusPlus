/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include <cstdint>
#include <functional>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <numeric_structs.h>
#include "blocks/block_properties.h"
#include "constants.h"

enum class ChunkState : uint8_t {
    Unloaded,
    Generating,
    Generated,
    Poulating,
    Populated,
    Lighting,
    Lit,
    Unloading
};

struct ChunkPos {
    int x = 0;
    int z = 0;
    bool operator==(const ChunkPos& other) const {
        return x == other.x && z == other.z;
    }

    uint64_t getHash() const {
        return (uint64_t(x) << 32) | uint64_t(z);
    }
};

template<>
struct std::hash<ChunkPos> {
    std::size_t operator()(const ChunkPos& p) const noexcept {
        return std::hash<uint64_t>{}(p.getHash());
    }
};

struct Chunk {
    static constexpr int VOLUME = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;
    static constexpr int META_VOLUME = VOLUME / 2;

    ChunkPos cpos;

    // Flat arrays — indexed by (y * CHUNK_WIDTH * CHUNK_WIDTH) + (z * CHUNK_WIDTH) + x
    // Equivalent to the old slice[y>>4].array[(y&15)<<8 | z<<4 | x], but with zero indirection.
    BlockType blocks[VOLUME] = { BLOCK_AIR };
    uint8_t   lightNibble[VOLUME] = { 0 };
    uint8_t   nibbleBlockMeta[META_VOLUME] = { 0 };

    std::atomic<ChunkState> state{ ChunkState::Unloaded };
    uint8_t heightMap[CHUNK_WIDTH * CHUNK_WIDTH] = {};
    float   temperature[CHUNK_WIDTH * CHUNK_WIDTH] = {};
    float   humidity[CHUNK_WIDTH * CHUNK_WIDTH] = {};

    bool isTerrainPopulated = false;
    bool isModified = false;
    bool spawnChunk = false;

    //  Index
    inline int blockIndex(Int3 pos) const {
        return (pos.y * CHUNK_WIDTH * CHUNK_WIDTH) + (pos.z * CHUNK_WIDTH) + pos.x;
    }

    //  Nibble helpers (unchanged from Slice)
    inline uint8_t setNibble(uint8_t hi, uint8_t lo) const {
        return uint8_t(((hi & 0x0Fu) << 4) | (lo & 0x0Fu));
    }
    inline uint8_t getNibbleLow(uint8_t byte) const { return  byte & 0x0Fu; }
    inline uint8_t getNibbleHigh(uint8_t byte) const { return (byte >> 4) & 0x0Fu; }

    //  Climate helpers
    //  NOTE: temperature/humidity use [(x << 4) | z] indexing — matching Java's
    //  WorldChunkManager.loadBlockGeneratorData fill order (outer X, inner Z) and
    //  the way these arrays are filled by BiomeGenerator. This deliberately differs
    //  from the heightMap convention below; Java uses different conventions for
    //  these two array types as well.
    inline float getTemperature(Int2 pos) const { return temperature[(pos.x << 4) | pos.y]; }
    inline float getHumidity(Int2 pos) const { return humidity[(pos.x << 4) | pos.y]; }

    //  Height map helpers
    inline uint8_t getHeightValue(Int2 pos) const {
        return heightMap[(pos.y << 4) | pos.x];
    }
    inline void setHeightValue(Int2 pos, uint8_t val) {
        heightMap[(pos.y << 4) | pos.x] = val;
    }
    inline bool canBlockSeeSky(Int3 pos) const {
        return pos.y >= getHeightValue({ pos.x, pos.z });
    }

    inline void generateHeightMap() {
        for (int x = 0; x < CHUNK_WIDTH; x++)
            for (int z = 0; z < CHUNK_WIDTH; z++)
                generateHeightMapColumn({ x, z });
    }

    inline void generateHeightMapColumn(Int2 pos) {
        for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
            if (Blocks::blockProperties[getBlock({ pos.x, y, pos.z })].lightOpacity > 0) {
                setHeightValue(pos, uint8_t(y + 1));
                return;
            }
        }
        setHeightValue(pos, 0);
    }

    inline void generateSkylightMap() {
        generateHeightMap();
        for (int x = 0; x < CHUNK_WIDTH; x++) {
            for (int z = 0; z < CHUNK_WIDTH; z++) {
                int height = getHeightValue({ x, z });
                for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
                    setSkyLight({ x, y, z }, 15);
                int skyLight = 15;
                for (int y = height - 1; y >= 0 && skyLight > 0; y--) {
                    skyLight -= std::max(1, int(Blocks::blockProperties[getBlock({ x, y, z })].lightOpacity));
                    if (skyLight > 0)
                        setSkyLight({ x, y, z }, uint8_t(skyLight));
                }
            }
        }
    }

    inline void relightColumn(Int2 pos) {
        generateHeightMapColumn(pos);
        int height = getHeightValue(pos);
        for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
            setSkyLight({ pos.x, y, pos.z }, 15);
        int skyLight = 15;
        for (int y = height - 1; y >= 0 && skyLight > 0; y--) {
            skyLight -= std::max(1, int(Blocks::blockProperties[getBlock({ pos.x, y, pos.z })].lightOpacity));
            if (skyLight > 0)
                setSkyLight({ pos.x, y, pos.z }, uint8_t(skyLight));
        }
    }

    //  Block helpers
    inline BlockType getBlock(Int3 pos) const {
        return blocks[blockIndex(pos)];
    }

    inline void setBlock(Int3 pos, BlockType id) {
        blocks[blockIndex(pos)] = id;
        isModified = true;
    }

    //  Meta helpers
    inline uint8_t getMeta(Int3 pos) const {
        int idx = blockIndex(pos);
        uint8_t byte = nibbleBlockMeta[idx >> 1];
        return (idx & 1) ? getNibbleHigh(byte) : getNibbleLow(byte);
    }

    inline void setMeta(Int3 pos, uint8_t meta) {
        int idx = blockIndex(pos);
        uint8_t& byte = nibbleBlockMeta[idx >> 1];
        byte = (idx & 1)
            ? setNibble(meta, getNibbleLow(byte))
            : setNibble(getNibbleHigh(byte), meta);
        isModified = true;
    }

    //  Light helpers
    inline uint8_t getBlockLight(Int3 pos) const {
        return getNibbleLow(lightNibble[blockIndex(pos)]);
    }

    inline uint8_t getSkyLight(Int3 pos) const {
        return getNibbleHigh(lightNibble[blockIndex(pos)]);
    }

    inline void setBlockLight(Int3 pos, uint8_t val) {
        uint8_t& byte = lightNibble[blockIndex(pos)];
        byte = setNibble(getNibbleHigh(byte), val);
        isModified = true;
    }

    inline void setSkyLight(Int3 pos, uint8_t val) {
        uint8_t& byte = lightNibble[blockIndex(pos)];
        byte = setNibble(val, getNibbleLow(byte));
        isModified = true;
    }

    inline int getBlockLightValue(Int3 pos, int skySubtracted) const {
        int sky = std::max(0, int(getSkyLight(pos)) - skySubtracted);
        int block = int(getBlockLight(pos));
        return std::min(15, std::max(sky, block));
    }

    //  Cleanup
    inline void clear() {
        isTerrainPopulated = false;
        isModified = false;
        std::memset(blocks, 0, sizeof(blocks));
        std::memset(lightNibble, 0, sizeof(lightNibble));
        std::memset(nibbleBlockMeta, 0, sizeof(nibbleBlockMeta));
        std::memset(heightMap, 0, sizeof(heightMap));
        std::memset(temperature, 0, sizeof(temperature));
        std::memset(humidity, 0, sizeof(humidity));
    }
};