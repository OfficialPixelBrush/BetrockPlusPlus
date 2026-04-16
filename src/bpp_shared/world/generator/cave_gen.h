/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once

#include "java_random.h"
#include "java_math.h"
#include "chunk.h"
#include "numeric_structs.h"

/**
 * @brief Used to carve caves into the world
 *
 */
class CaveGenerator {
private:
	const int32_t carveExtentLimit = 8;
	Java::Random rand = Java::Random();

public:
	CaveGenerator();
	void GenerateCavesForChunk(Chunk& chunk, int64_t seed);
	void GenerateCaves(Chunk& chunk, Int2 chunkOffset);
	void CarveCave(Chunk& chunk, Vec3 offset);
	void CarveCave(Chunk& chunk, Vec3 offset,
		float tunnelRadius, float carveYaw, float carvePitch,
		int32_t tunnelStep, int32_t tunnelLength,
		double verticalScale);
};