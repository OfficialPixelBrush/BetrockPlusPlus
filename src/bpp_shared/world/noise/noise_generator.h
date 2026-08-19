/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include "java_random.h"
#include "numeric_structs.h"
#include <array>
#include <vector>

#ifdef REDUCED_GENERATION_PRECISION
typedef float gen_float;
#define GenFloatToInt32 Java::FloatToInt32
#else
typedef double gen_float;
#define GenFloatToInt32 Java::DoubleToInt32
#endif

/**
 * @brief The base Noise generator object that splits into Perlin and Simplex noise
 * 
 */
class NoiseGenerator {
protected:
	uint8_t permutations[512];
	Vec3 coordinate;
	virtual void InitPermTable(Java::Random& _rand);

public:
	NoiseGenerator();
	NoiseGenerator(Java::Random& _rand);

	virtual ~NoiseGenerator() = default;
};