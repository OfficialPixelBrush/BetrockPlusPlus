/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include <vector>
#include <libdeflate.h>
#include "chunk.h"

namespace ChunkSerializer {
    inline std::vector<uint8_t> serialize(const Chunk& chunk) {
        constexpr int BLOCKS = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;
        constexpr int NIBBLES = BLOCKS / 2;
        constexpr int TOTAL = BLOCKS + NIBBLES * 3;

        static_assert(TOTAL == 81920, "Chunk data size mismatch");

        std::vector<uint8_t> raw(TOTAL, 0);
        uint8_t* blockData = raw.data();
        uint8_t* metaData = blockData + BLOCKS;
        uint8_t* blockLight = metaData + NIBBLES;
        uint8_t* skyLight = blockLight + NIBBLES;

        for (int x = 0; x < CHUNK_WIDTH; x++) {
            for (int z = 0; z < CHUNK_WIDTH; z++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    int  idx = (x << 11) | (z << 7) | y;
                    int  nibbleIdx = idx >> 1;
                    bool highNibble = idx & 1;
                    Int3 pos{ x, y, z };

                    blockData[idx] = uint8_t(chunk.getBlock(pos));

                    auto packNibble = [](uint8_t& byte, uint8_t val, bool high) {
                        if (high) byte = uint8_t((byte & (uint8_t(0x0F))) | ((val & uint8_t(0x0F)) << 4));
                        else      byte = uint8_t((byte & uint8_t(0xF0)) | (val & uint8_t(0x0F)));
                        };

                    packNibble(metaData[nibbleIdx], chunk.getMeta(pos), highNibble);
                    packNibble(blockLight[nibbleIdx], chunk.getBlockLight(pos), highNibble);
                    packNibble(skyLight[nibbleIdx], chunk.getSkyLight(pos), highNibble);
                }
            }
        }

        libdeflate_compressor* compressor = libdeflate_alloc_compressor(6);
        size_t maxSize = libdeflate_zlib_compress_bound(compressor, TOTAL);
        std::vector<uint8_t> compressed(maxSize);
        size_t actualSize = libdeflate_zlib_compress(compressor, raw.data(), TOTAL, compressed.data(), maxSize);
        libdeflate_free_compressor(compressor);
        compressed.resize(actualSize);
        return compressed;
    }
}