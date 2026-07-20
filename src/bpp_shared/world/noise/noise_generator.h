/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include "java_random.h"
#include "numeric_structs.h"

/**
 * @brief The base Noise generator object that splits into Perlin and Simplex noise
 * 
 */
class NoiseGenerator {
protected:
	int32_t permutations[512];
	Vec3 coordinate;
	double GenerateNoiseBase(Vec3 p_offset);
	void InitPermTable(Java::Random& rand);

public:
	NoiseGenerator();
	NoiseGenerator(Java::Random& rand);

	virtual ~NoiseGenerator() = default;
};