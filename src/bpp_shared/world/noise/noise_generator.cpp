/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#include "noise_generator.h"

NoiseGenerator::NoiseGenerator() {}

NoiseGenerator::NoiseGenerator([[maybe_unused]]Java::Random& rand) { VEC3_ZERO; }

double NoiseGenerator::GenerateNoise([[maybe_unused]] Vec3 p_coordinate) { return 0.0; }
double NoiseGenerator::GenerateNoise([[maybe_unused]] Vec2 p_coordinate) { return 0.0; }
void NoiseGenerator::GenerateNoise([[maybe_unused]] std::vector<double> &values, [[maybe_unused]] Vec3 p_offset, [[maybe_unused]] Int32_3 p_size, [[maybe_unused]] Vec3 p_scale, [[maybe_unused]] double amplitude) {}
void NoiseGenerator::GenerateNoise([[maybe_unused]] std::vector<double> &values, [[maybe_unused]] Vec2 p_offset, [[maybe_unused]] Int32_2 p_size, [[maybe_unused]] Vec2 p_scale, [[maybe_unused]] double amplitude) {}