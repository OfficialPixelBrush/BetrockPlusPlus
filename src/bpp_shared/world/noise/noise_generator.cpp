/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#include "noise_generator.h"

NoiseGenerator::NoiseGenerator() {}

NoiseGenerator::NoiseGenerator([[maybe_unused]]Java::Random& rand) { VEC3_ZERO; }

double NoiseGenerator::GenerateNoise([[maybe_unused]] double x, [[maybe_unused]] double y) { return 0; }

double NoiseGenerator::GenerateNoise([[maybe_unused]] double x, [[maybe_unused]] double y, [[maybe_unused]] double z) {
	return 0;
}
