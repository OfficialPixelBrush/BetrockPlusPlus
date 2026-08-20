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

#if defined(GENERATION_PRECISION_FLOAT64)
typedef double gen_float;
#define GenFloatToInt32 Java::DoubleToInt32
#elif defined(GENERATION_PRECISION_FLOAT32)
typedef float gen_float;
#define GenFloatToInt32 Java::FloatToInt32
#elif defined(GENERATION_PRECISION_INT64)
#include "../../quantized_types.h"
#include <cstdint>
typedef Fixed<int64_t, (1 << 28)> gen_float;
// Toward-zero conversion, matching Java's FloatToInt32 / DoubleToInt32.
constexpr inline int32_t GenFloatToInt32(const gen_float value) {
    return static_cast<int32_t>(value.Raw() / gen_float::M_SCALE);
}
#elif defined(GENERATION_PRECISION_INT32)
#include "../../quantized_types.h"
#include <cstdint>
typedef Fixed<int32_t, (1 << 14)> gen_float;
// Toward-zero conversion, matching Java's FloatToInt32 / DoubleToInt32.
constexpr inline int32_t GenFloatToInt32(const gen_float value) {
    return static_cast<int32_t>(value.Raw() / gen_float::M_SCALE);
}
#else
#error "Unsupported GENERATION_PRECISION"
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