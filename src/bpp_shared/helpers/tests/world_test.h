/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "../../world/chunk.h"
#include "world_test_data.h"
#pragma once

class UnitTest {
    public:
        float CompareBlocks(Chunk& chunk, uint8_t b[], bool diff_info){
            int matchingBlocks = 0;
            for (int y = 0; y < 128; y++) 
                for (int z = 0; z < 16; z++)
                    for (int x = 0; x < 16; x++){
                        auto block = chunk.getBlock({x, y, z});
                        if (BlockType(block) == BlockType(b[chunk.blockIndex({x,y,z})]))
                            matchingBlocks++;
                        else {
                            chunk.setBlock(Int3(x,y,z), BLOCK_BRICKS);
                            if (diff_info)
                            std::cout << Int3(x,y,z) << ":\t" << IdToLabel(block) << " =/= " << IdToLabel(int16_t(b[chunk.blockIndex({x,y,z})])) << "\n";
                        }
                    }
            return (float(matchingBlocks)/float(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH))*100.0f;
        }
};

UnitTest ut;