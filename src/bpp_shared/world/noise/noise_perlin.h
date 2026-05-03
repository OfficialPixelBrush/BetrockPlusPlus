/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

// A recreation of the the Infdev 20100227-1433 Perlin noise function
#pragma once
#include "noise_generator.h"
#include "numeric_structs.h"
#include "java_math.h"


/**
 * @brief A faithful reimplementation of the Infdev and Beta perlin noise generator
 * 
 */
class NoisePerlin : public NoiseGenerator {
  protected:
	int32_t permutations[512];
	Vec3 coordinate;
	double GenerateNoiseBase(Vec3 pos);
	void InitPermTable(Java::Random& rand);

  public:
	NoisePerlin();
	NoisePerlin(Java::Random& rand);
	double GenerateNoise(Vec2 coord);
	double GenerateNoise(Vec3 coord);
	void GenerateNoise(std::vector<double> &noiseField, Vec3 offset, Int3 size, Vec3 scale, double amplitude);
};