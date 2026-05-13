/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "chunk.h"
#include "numeric_structs.h"
#include <memory>

#define REGION_WIDTH 32
#define SECTOR_SIZE 4096

class Region {
    public:
        Region(Int2 rpos);
        bool Serialize();
        bool Deserialize();

    private:    
        // TODO: Could probably be an array, since it has a fixed max size?
        std::unordered_map<ChunkPos, std::shared_ptr<Chunk>> chunks;
        Int2 rpos;
};