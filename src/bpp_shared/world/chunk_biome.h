/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */

#pragma once
#include "constants.h"
#include "helpers/packed_array.h"

// Per-column climate and biome data for a chunk. Kept out of Chunk so it can be
// allocated lazily (the Nether never needs it).
struct ChunkBiome {
	float temperature[CHUNK_AREA] = {};
	float humidity[CHUNK_AREA] = {};
	PackedArray<CHUNK_AREA, 4> biomes;
};
