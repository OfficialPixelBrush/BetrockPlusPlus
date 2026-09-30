/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */

#pragma once
#include "constants.h"
#include "helpers/packed_array.h"

// Per-column climate, allocated lazily, since the Nether never uses it,
// and already generated chunks don't need it
struct ChunkClimate {
	float temperature[CHUNK_AREA] = {};
	float humidity[CHUNK_AREA] = {};
	PackedArray<CHUNK_AREA, 4> biomes;
};
