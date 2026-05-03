/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

// A recreation of the the Infdev 20100227-1433 Perlin noise function
#pragma once
#include "noise_generator.h"

/**
 * @brief A faithful reimplementation of the Beta-era simplex noise generator, often used for Biome generation
 * 
 */
class NoiseSimplex : public NoiseGenerator {
  protected:
	int32_t permutations[512];
	Vec3 coordinate;
	double GenerateNoiseBase(Vec3 position);
	void InitPermTable(Java::Random& rand);

  private:
	int32_t gradients[12][3] = {{1, 1, 0},	{-1, 1, 0},	 {1, -1, 0}, {-1, -1, 0}, {1, 0, 1},  {-1, 0, 1},
							{1, 0, -1}, {-1, 0, -1}, {0, 1, 1},	 {0, -1, 1},  {0, 1, -1}, {0, -1, -1}};
	double skewing = 0.5 * (sqrt(3.0) - 1.0);
	double unskewing = (3.0 - sqrt(3.0)) / 6.0;

  public:
	NoiseSimplex();
	NoiseSimplex(Java::Random& rand);
	~NoiseSimplex() override {}
	void GenerateNoise(std::vector<double> &values, Vec2 p_coordinate, Int32_2 p_size, Vec2 p_scale, double amplitude) override;
};

inline int32_t wrap(double grad) { return grad > 0.0 ? Java::DoubleToInt32(grad) : Java::DoubleToInt32(grad) - 1; }

inline double dotProd(int32_t grad[3], double x, double y) { return double(grad[0]) * x + double(grad[1]) * y; }