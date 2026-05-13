/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "region.h"
#include "logger.h"
#include <fstream>
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

void Region::AddChunk(std::shared_ptr<Chunk>& chunk) {
    chunks[GetChunkHeaderOffset(chunk->cpos)] = chunk;
}

bool Region::Serialize() {
    std::ofstream file(
        "r." +
        std::to_string(rpos.x) +
        "." +
        std::to_string(rpos.z) +
        ".mcr"
    );
    if (!file.is_open()) {
        GlobalLogger().warn << "Failed to save region! " << this->rpos << "\n";
    }
    size_t sectorIndex = 0;
    std::array<uint32_t, REGION_AREA> chunkSector;
    sectorIndex+=2;
    
    // TODO We skip the timestamp info for now
    for (const auto& cnk : chunks) {
        if (!cnk) continue;
        // Move to sector position
        file.seekp(sectorIndex * SECTOR_SIZE);
        // Write position of this chunk
        chunkSector[GetChunkHeaderOffset(cnk->cpos)] = (sectorIndex * SECTOR_SIZE);
        // TODO: Put in actual NBT data
        auto data = ChunkSerializer::serialize(*cnk);
        // Determine amount of sectors this'll take
        size_t writenSectors = (data.size() / SECTOR_SIZE) + 1;
        file << data.data();
        sectorIndex += writenSectors;
    }
    file.seekp(0);
    // Write header position
    for (auto& sector : chunkSector) {
        sector = __builtin_bswap32(sector);
        file.write(reinterpret_cast<const char*>(&sector), sizeof(sector));
    }
    file.close();
}

bool Region::Deserialize() {
    // TODO
}