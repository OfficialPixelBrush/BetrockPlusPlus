/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "chunk.h"
#include "numeric_structs.h"
#include <memory>

#define REGION_WIDTH 32
#define REGION_AREA REGION_WIDTH*REGION_WIDTH
#define SECTOR_SIZE 4096

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
        Region(Int32_2 rpos);
        void AddChunk(std::shared_ptr<Chunk>& chunk);
        bool Serialize();
        bool Deserialize();

    private:
        std::array<std::shared_ptr<Chunk>, REGION_AREA> chunks;
        Int32_2 rpos;
        std::vector<uint8_t> EncodeNbtData(const std::shared_ptr<Chunk>& chunk);
        std::shared_ptr<Chunk> DecodeNbtData(const std::vector<uint8_t>& data);
        std::string GetPath();
};