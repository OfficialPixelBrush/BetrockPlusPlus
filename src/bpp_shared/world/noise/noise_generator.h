/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include "java_math.h"
#include "java_random.h"
#include "numeric_structs.h"
#include <cmath>
#include <vector>

/**
 * @brief The base Noise generator object that splits into Perlin and Simplex noise
 * 
 */
class NoiseGenerator {
  private:
	int32_t permutations[512];
	Vec3 coordinate;
	double GenerateNoiseBase(double x, double y, double z);
	void InitPermTable(Java::Random& rand);

  public:
	NoiseGenerator();
	NoiseGenerator(Java::Random& rand);
	double GenerateNoise(double x, double y);
	double GenerateNoise(double x, double y, double z);
	void GenerateNoise(std::vector<double> &var1, double var2, double var4, double var6, int32_t var8, int32_t var9, int32_t var10,
					   double var11, double var13, double var15, double var17);
};