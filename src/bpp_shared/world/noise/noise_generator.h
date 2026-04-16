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
	double GenerateNoiseBase(Vec3 p_offset);
	void InitPermTable(Java::Random& rand);

  public:
	NoiseGenerator();
	NoiseGenerator(Java::Random& rand);

	virtual ~NoiseGenerator() = default;

	virtual double GenerateNoise(Vec2 p_offset);
	virtual double GenerateNoise(Vec3 p_offset);
	
	virtual void GenerateNoise(std::vector<double> &values, Vec3 p_offset, Int32_3 p_size, Vec3 p_scale, double amplitude);
	virtual void GenerateNoise(std::vector<double> &values, Vec2 p_offset, Int32_2 p_size, Vec2 p_scale, double amplitude);
};