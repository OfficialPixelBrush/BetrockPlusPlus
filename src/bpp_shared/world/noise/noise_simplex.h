/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

// A recreation of the the Infdev 20100227-1433 Perlin noise function
#pragma once
#include "java_math.h"
#include "noise_generator.h"
#include <cmath>

/**
 * @brief A faithful reimplementation of the Beta-era simplex noise generator, often used for Biome generation
 * 
 */
class NoiseSimplex : public NoiseGenerator {
protected:
	int32_t permutations[512];
	Vec3 coordinate;
	double GenerateNoiseBase(Vec3 _position);
	void InitPermTable(Java::Random& _rand);

private:
	int32_t gradients[12][3] = { { 1, 1, 0 },  { -1, 1, 0 },  { 1, -1, 0 }, { -1, -1, 0 }, { 1, 0, 1 },  { -1, 0, 1 },
		                         { 1, 0, -1 }, { -1, 0, -1 }, { 0, 1, 1 },  { 0, -1, 1 },  { 0, 1, -1 }, { 0, -1, -1 } };
	double skewing = 0.5 * (sqrt(3.0) - 1.0);
	double unskewing = (3.0 - sqrt(3.0)) / 6.0;

public:
	NoiseSimplex();
	NoiseSimplex(Java::Random& _rand);
	~NoiseSimplex() override {}
	void GenerateNoise(std::vector<double>& _values, Vec2 _pCoordinate, Int32_2 _pSize, Vec2 _pScale, double _amplitude);
};

inline int32_t Wrap(double _grad) {
	return _grad > 0.0 ? Java::DoubleToInt32(_grad) : Java::DoubleToInt32(_grad) - 1;
}

inline double DotProd(int32_t _grad[3], double _x, double _y) {
	return double(_grad[0]) * _x + double(_grad[1]) * _y;
}