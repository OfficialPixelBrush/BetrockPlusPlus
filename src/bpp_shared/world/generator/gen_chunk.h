/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include "constants.h"
#include "helper/packed_array.h"

// A 16x16x128 chunk that's fully allocated as a set of plain arrays
// This will hopefully speed up terrain generation/access
struct GenChunk {
    std::array<BlockType, CHUNK_VOLUME> block;
    PackedArray<CHUNK_VOLUME, 4> meta;
    PackedArray<CHUNK_VOLUME, 4> blockLight;
    PackedArray<CHUNK_VOLUME, 4> skyLight;
};