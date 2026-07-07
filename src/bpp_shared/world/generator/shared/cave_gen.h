/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once

#include "chunk.h"
#include "constants.h"
#include "java_random.h"
#include "numeric_structs.h"

/**
 * @brief Used to carve caves into the world
 *
 */
class CaveGenerator {
private:
	static constexpr int32_t m_carveExtentLimit = 8;
	Java::Random m_rand = Java::Random();

public:
	CaveGenerator(bool isNetherCave = false) : m_isNetherCave(isNetherCave) {}
	void GenerateCavesForChunk(Chunk& chunk, int64_t seed);
	void GenerateCaves(Chunk& chunk, Int2 chunkOffset);
	void CarveCave(Chunk& chunk, Vec3 offset);
	void CarveCave(Chunk& chunk, Vec3 offset, float tunnelRadius, float carveYaw, float carvePitch, int32_t tunnelStep,
	               int32_t tunnelLength, double verticalScale);
	bool m_isNetherCave = false;
};