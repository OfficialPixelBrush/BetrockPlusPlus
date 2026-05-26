/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "chunk.h"
#include "numeric_structs.h"
#include "helpers/file_handle.h"
#include "helpers/byteswap_compat.h"
#include <atomic>
#include <memory>
#include <mutex>

#define REGION_WIDTH 32
#define REGION_AREA REGION_WIDTH*REGION_WIDTH
#define SECTOR_SIZE 4096

inline std::string regionPositionToFileName(Int2 rpos) {
    return "r." + std::to_string(rpos.x) + "." + std::to_string(rpos.z) + ".mcr";
}

enum CompressorFormat {
    REGION_INVALID = 0,
    REGION_GZIP = 1,
    REGION_ZLIB = 2
};

struct FileHeaderEntry {
    uint32_t offset;
    uint8_t numberOfSectors;
    // TODO: Maybe store last-updated here?
};

struct ChunkHeaderEntry {
    uint32_t length;
    uint8_t format;
};

class Region {
public:
    Int32_2 m_rpos;
    std::mutex m_mutex;
    Region(Int32_2 rpos, std::string folderPath)
        : m_rpos(rpos)
        , regionFile(folderPath + "/" + regionPositionToFileName(rpos))
    {
        // Cache our header
        readHeaderFromFile();
    }
    bool chunkExists(Int2 localcpos) {
        int index = localcpos.x + localcpos.z * 32;
        auto* rHeader = &regionHeader[index];
        return (rHeader->numberOfSectors != 0 && rHeader->offset != 0);
    }

    void AddChunk(std::shared_ptr<Chunk> chunk);
    std::shared_ptr<Chunk> GetChunk(Int32_2 cpos);

    // Read our header data into the "regionHeader"
    void readHeaderFromFile() {
        auto& file = regionFile.get();
        file.seekg(0); // Beginning of sector 0
        for (int i = 0; i < 1024; i++) {
            uint32_t entry;
            file.read(reinterpret_cast<char*>(&entry), 4);
            entry = __builtin_bswap32(entry);
            regionHeader[i].numberOfSectors = entry & 0xFF; // bottom 1 byte
            regionHeader[i].offset = entry >> 8; // top 3 bytes
        }
    }

private:
    std::array<std::shared_ptr<Chunk>, REGION_AREA> chunks;
    std::vector<uint8_t> EncodeNbtData(const std::shared_ptr<Chunk>& chunk);
    std::shared_ptr<Chunk> DecodeNbtData(const std::vector<uint8_t>& raw_data);
    std::string GetPath();
    std::array<FileHeaderEntry, 1024> regionHeader;
    FileHandle regionFile;
};