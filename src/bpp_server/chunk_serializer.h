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
    inline std::vector<uint8_t> serialize(const Chunk& chunk, int xmin = 0, int xmax = 16, int ymin = 0, int ymax = 128, int zmin = 0, int zmax = 16) {
        int sizeX = xmax - xmin;
        int sizeY = ymax - ymin;
        int sizeZ = zmax - zmin;

        int blocks = sizeX * sizeY * sizeZ;
        int nibbles = (blocks + 1) / 2;
        int total = blocks + nibbles * 3;

        std::vector<uint8_t> raw(size_t(total), 0);
        uint8_t* blockData = raw.data();
        uint8_t* metaData = blockData + blocks;
        uint8_t* blockLight = metaData + nibbles;
        uint8_t* skyLight = blockLight + nibbles;

        auto packNibble = [](uint8_t& byte, uint8_t val, bool high) {
            if (high) byte = uint8_t((byte & 0x0F) | ((val & 0x0F) << 4));
            else      byte = uint8_t((byte & 0xF0) | (val & 0x0F));
            };

        int i = 0;
        for (int x = xmin; x < xmax; x++) {
            for (int z = zmin; z < zmax; z++) {
                for (int y = ymin; y < ymax; y++, i++) {
                    Int3 pos{ x, y, z };
                    blockData[i] = uint8_t(chunk.getBlock(pos));
                    packNibble(metaData[i >> 1], chunk.getMeta(pos), i & 1);
                    packNibble(blockLight[i >> 1], chunk.getBlockLight(pos), i & 1);
                    packNibble(skyLight[i >> 1], chunk.getSkyLight(pos), i & 1);
                }
            }
        }

        libdeflate_compressor* compressor = libdeflate_alloc_compressor(6);
        if (!compressor) return {};
        size_t maxSize = libdeflate_zlib_compress_bound(compressor, static_cast<size_t>(total));
        std::vector<uint8_t> compressed(maxSize);
        size_t actualSize = libdeflate_zlib_compress(compressor, raw.data(), static_cast<size_t>(total), compressed.data(), maxSize);
        libdeflate_free_compressor(compressor);
        compressed.resize(actualSize);
        return compressed;
    }
}