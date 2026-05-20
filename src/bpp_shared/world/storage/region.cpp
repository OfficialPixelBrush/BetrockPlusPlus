/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "region.h"
#include "nbt/nbt.h"
#include "chunk.h"
#include "logger.h"
#include "helpers/byteswap_compat.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <libdeflate.h>
#include <memory>
#include <string>
#include "../../../bpp_server/chunk_serializer.h"

Region::Region(Int2 prpos) {
    this->rpos = prpos;
}

size_t GetChunkHeaderOffset(Int32_2 cpos) {
    return size_t((cpos.x % REGION_WIDTH) +
                 ((cpos.z % REGION_WIDTH) * REGION_WIDTH));
}

Int32_2 GetChunkHeaderPosition(size_t cidx) {
    return Int32_2 {
        int32_t(cidx % REGION_WIDTH),
        int32_t(cidx / REGION_WIDTH),
    };
}

void Region::AddChunk(std::shared_ptr<Chunk> chunk) {
    chunks[GetChunkHeaderOffset(chunk->cpos)] = chunk;
}

std::shared_ptr<Chunk> Region::GetChunk(Int32_2 cpos) {
    return chunks[GetChunkHeaderOffset(cpos)];
}

std::vector<uint8_t> Region::EncodeNbtData(const std::shared_ptr<Chunk>& chunk) {
    // Build a compound tag representing a chunk level entry
    Tag root;
    root.type = TAG_COMPOUND;
    root.name = "";

    Tag level;
    level.type = TAG_COMPOUND;
    level.name = "Level";

    // Scalar values
    Tag xPos;  xPos.type = TAG_INT;  xPos.name = "xPos";  xPos.intValue = chunk->cpos.x;
    Tag zPos;  zPos.type = TAG_INT;  zPos.name = "zPos";  zPos.intValue = chunk->cpos.z;
    Tag populated; populated.type = TAG_BYTE; populated.name = "TerrainPopulated"; populated.byteValue = chunk->isTerrainPopulated;
    Tag lastUpdate; lastUpdate.type = TAG_LONG; lastUpdate.name = "LastUpdate"; lastUpdate.longValue = 123456789LL;

    // Byte array — blocks
    Tag blocks;
    blocks.type = TAG_BYTEARRAY;
    blocks.name = "Blocks";
    blocks.byteArray.resize((CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT), 0);

    // Nibble arrays (4 bits per block, so half the size of blocks array)
    Tag data;
    data.type = TAG_BYTEARRAY;
    data.name = "Data";
    data.byteArray.resize((CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT) / 2, 0);

    Tag blockLight;
    blockLight.type = TAG_BYTEARRAY;
    blockLight.name = "BlockLight";
    blockLight.byteArray.resize((CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT) / 2, 0);

    Tag skyLight;
    skyLight.type = TAG_BYTEARRAY;
    skyLight.name = "SkyLight";
    skyLight.byteArray.resize((CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT) / 2, 0);

    // HeightMap — one byte per (x,z) column
    Tag heightMap;
    heightMap.type = TAG_BYTEARRAY;
    heightMap.name = "HeightMap";
    heightMap.byteArray.resize(CHUNK_WIDTH * CHUNK_WIDTH, 0);
    std::copy(
        std::begin(chunk->heightMap),
        std::end(chunk->heightMap),
        heightMap.byteArray.data()
    );

    // Put blocks in there
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                size_t idx = size_t(y + (z * CHUNK_HEIGHT) + (x * CHUNK_HEIGHT * CHUNK_WIDTH));
                blocks.byteArray[idx] = chunk->getBlock({x,y,z});
                if (y % 2 == 0) {
                    data.byteArray[idx/2] |= (chunk->getMeta({x,y,z}));
                    skyLight.byteArray[idx/2] |= chunk->getSkyLight({x,y,z});
                    blockLight.byteArray[idx/2] |= chunk->getBlockLight({x,y,z});
                } else {
                    data.byteArray[idx/2] |= (chunk->getMeta({x,y,z}) << 4);
                    skyLight.byteArray[idx/2] |= (chunk->getSkyLight({x,y,z}) << 4);
                    blockLight.byteArray[idx/2] |= (chunk->getBlockLight({x,y,z}) << 4);
                }
            }
        }
    }

    // List tag — entities (empty)
    Tag entities;
    entities.type = TAG_LIST;
    entities.name = "Entities";
    entities.listType = TAG_COMPOUND;

    // Nested compound inside a list — tile entities
    Tag tileEntities;
    tileEntities.type = TAG_LIST;
    tileEntities.name = "TileEntities";
    tileEntities.listType = TAG_COMPOUND;

    // Assemble level compound
    level.compound["xPos"]             = xPos;
    level.compound["zPos"]             = zPos;
    level.compound["TerrainPopulated"] = populated;
    level.compound["LastUpdate"]       = lastUpdate;
    level.compound["Blocks"]           = blocks;
    level.compound["Data"]             = data;
    level.compound["BlockLight"]       = blockLight;
    level.compound["SkyLight"]         = skyLight;
    level.compound["HeightMap"]        = heightMap;
    level.compound["Entities"]         = entities;
    level.compound["TileEntities"]     = tileEntities;

    root.compound["Level"] = level;

    // Serialize to bytes
    std::vector<uint8_t> raw;
    NBTwriter writer(raw, root);
    // Compress
    libdeflate_compressor* compressor = libdeflate_alloc_compressor(6);
    if (!compressor) return {};
    size_t maxSize = libdeflate_zlib_compress_bound(compressor, static_cast<size_t>(raw.size()));
    std::vector<uint8_t> compressed(maxSize);
    size_t actualSize = libdeflate_zlib_compress(compressor, raw.data(), static_cast<size_t>(raw.size()), compressed.data(), maxSize);
    libdeflate_free_compressor(compressor);
    compressed.resize(actualSize);
    return compressed;
}

// TODO
std::shared_ptr<Chunk> Region::DecodeNbtData(const std::vector<uint8_t>& raw_data) {
    libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();
    if (!decompressor) return std::make_shared<Chunk>();
    size_t decompressedSize = (16*128*16) * 2.5;
    std::vector<uint8_t> decompressed(decompressedSize);
    while (true) {
        decompressed.resize(decompressedSize);
        size_t actualOutSize = 0;
        libdeflate_result result =
            libdeflate_zlib_decompress(
                decompressor,
                raw_data.data(),
                raw_data.size(),
                decompressed.data(),
                decompressed.size(),
                &actualOutSize
            );

        if (result == LIBDEFLATE_SUCCESS) {
            decompressed.resize(actualOutSize);
            break;
        }

        if (result != LIBDEFLATE_INSUFFICIENT_SPACE) {
            GlobalLogger().warn << "Decompression failed!\n";
            break;
        }

        decompressedSize *= 1.5;
    }
    libdeflate_free_decompressor(decompressor);
    NBTParser parser(decompressed.data(), int64_t(decompressed.size()));
    const Tag& lvl = parser.root.get("Level");

    int32_t cx = lvl.get("xPos").getInt();
    int32_t cz = lvl.get("zPos").getInt();
    bool    tp = lvl.get("TerrainPopulated").getByte() != 0;
    int64_t lu = lvl.get("LastUpdate").getLong();

    const auto& blocks = lvl.get("Blocks").getByteArray();
    const auto& data = lvl.get("Data").getByteArray();
    const auto& blockLight = lvl.get("BlockLight").getByteArray();
    const auto& skyLight = lvl.get("SkyLight").getByteArray();
    const auto& heightMap = lvl.get("HeightMap").getByteArray();
    auto chunk = std::make_shared<Chunk>();

    chunk->cpos = Int32_2{cx,cz};
    chunk->state = tp ? ChunkState::Populated : ChunkState::Generated;
    std::copy(
        heightMap.begin(),
        heightMap.end(),
        chunk->heightMap
    );

    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        for (int x = 0; x < CHUNK_WIDTH; x++) {
            for (int z = 0; z < CHUNK_WIDTH; z++) {
                size_t idx = size_t(y + (z * CHUNK_HEIGHT) + (x * CHUNK_HEIGHT * CHUNK_WIDTH));
                chunk->setBlock({x,y,z}, BlockType(blocks[idx]));
                if (y % 2 == 0) {
                    chunk->setMeta({x,y,z}, data[idx/2] & 0xF);
                    chunk->setBlockLight({x,y,z}, blockLight[idx/2] & 0xF);
                    chunk->setSkyLight({x,y,z}, skyLight[idx/2] & 0xF);
                } else {
                    chunk->setMeta({x,y,z}, (data[idx/2] >> 4) & 0xF);
                    chunk->setBlockLight({x,y,z}, (blockLight[idx/2] >> 4) & 0xF);
                    chunk->setSkyLight({x,y,z}, (skyLight[idx/2] >> 4) & 0xF);
                }
            }
        }
    }
    return chunk;
}

inline std::string Region::GetPath() {
    return
        "region/r." +
        std::to_string(rpos.x) +
        "." +
        std::to_string(rpos.z) +
        ".mcr";
}

// Turn region into mcr file
bool Region::Serialize() {
    if (!std::filesystem::exists("region"))
        std::filesystem::create_directory("region");
    std::ofstream file(GetPath(), std::ios::binary);
    if (!file.is_open()) {
        GlobalLogger().warn << "Failed to save region! " << this->rpos << "\n";
        return false;
    }

    // Sector 0: chunk location table  (4096 bytes)
    // Sector 1: chunk timestamp table (4096 bytes)
    // Both are written at the end / zeroed now so the file has correct structure
    // from the start even if we seek around.
    constexpr size_t HEADER_SECTORS = 2;
    size_t sectorIndex = HEADER_SECTORS;

    std::array<uint32_t, REGION_AREA> chunkSector{};   // location table entries
    std::array<uint32_t, REGION_AREA> chunkTimestamp{}; // timestamp table (zeroed)

    for (const auto& cnk : chunks) {
        if (!cnk) continue;

        auto data = EncodeNbtData(cnk);
        if (data.empty()) {
            GlobalLogger().warn << "Failed to compress " << cnk->cpos << "!\n";
            continue;
        }

        // Move to sector position
        file.seekp(static_cast<std::streamoff>(sectorIndex * SECTOR_SIZE));

        // Total bytes on disk: 4-byte length + 1-byte compression type + payload
        size_t totalBytes    = sizeof(uint32_t) + sizeof(uint8_t) + data.size();
        size_t writtenSectors = (totalBytes + SECTOR_SIZE - 1) / SECTOR_SIZE;

        // Record location using region-local chunk coordinates
        chunkSector[GetChunkHeaderOffset(cnk->cpos)] =
            (static_cast<uint32_t>(sectorIndex) << 8) | static_cast<uint32_t>(writtenSectors);

        // Write length (big-endian): counts compression-type byte + payload
        uint32_t payloadLength = __builtin_bswap32(static_cast<uint32_t>(data.size() + sizeof(uint8_t)));
        file.write(reinterpret_cast<const char*>(&payloadLength), sizeof(payloadLength));

        // Compression type: 2 = zlib
        uint8_t compressionType = 2;
        file.write(reinterpret_cast<const char*>(&compressionType), sizeof(compressionType));

        // Compressed chunk data
        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

        // Pad to the next sector boundary
        size_t padding = writtenSectors * SECTOR_SIZE - totalBytes;
        if (padding > 0) {
            std::vector<char> zeros(padding, 0);
            file.write(zeros.data(), static_cast<std::streamsize>(zeros.size()));
        }

        sectorIndex += writtenSectors;
    }

    // Write sector 0: location table
    file.seekp(0);
    for (auto entry : chunkSector) {
        uint32_t be = __builtin_bswap32(entry);
        file.write(reinterpret_cast<const char*>(&be), sizeof(be));
    }

    // Write sector 1: timestamp table (all zeros)
    // seekp should already be at byte 4096 after writing 1024 × 4-byte entries,
    // but seek explicitly to be safe.
    file.seekp(static_cast<std::streamoff>(SECTOR_SIZE));
    for (auto ts : chunkTimestamp) {
        uint32_t be = __builtin_bswap32(ts);
        file.write(reinterpret_cast<const char*>(&be), sizeof(be));
    }

    file.close();
    return true;
}

// Turn mcr into Region
bool Region::Deserialize() {
    // Region doesn't exist, fail
    if (!std::filesystem::exists(GetPath()))
        return false;
    // Deserialize Region
    std::ifstream file(GetPath(), std::ios::binary);
    if (!file.is_open()) {
        GlobalLogger().warn << "Failed to open region " << rpos << "!\n";
        return false;
    }

    // Read the header
    std::vector<FileHeaderEntry> chunkSector;
    for (int i = 0; i < REGION_AREA; i++) {
        // Read sector offset
        uint32_t be = 0;
        file.read(reinterpret_cast<char*>(&be), sizeof(be));
        // No data, skip
        if (be == 0)
            continue;
        be = __builtin_bswap32(be);
        FileHeaderEntry fhe {
            be >> 8,
            static_cast<uint8_t>(be & 0xFF)
        };
        chunkSector.push_back(fhe);
    }

    // Read the found sectors in
    for (const FileHeaderEntry& fhe : chunkSector) {
        file.seekg(fhe.offset * SECTOR_SIZE);
        // Read Chunk Header
        ChunkHeaderEntry che;
        file.read(
            reinterpret_cast<char*>(&che.length),
            sizeof(che.length)
        );
        che.length = __builtin_bswap32(che.length);
        file.read(
            reinterpret_cast<char*>(&che.format),
            sizeof(che.format)
        );

        // Read raw data
        std::vector<uint8_t> raw_data;
        raw_data.resize(fhe.numberOfSectors * SECTOR_SIZE);
        file.read(
            reinterpret_cast<char*>(raw_data.data()),
            static_cast<std::streamsize>(che.length - 1)
        );
        auto chunk = DecodeNbtData(raw_data);
        AddChunk(chunk);
    }

    file.close();
    return true;
}