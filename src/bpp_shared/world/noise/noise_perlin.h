/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

// A recreation of the the Infdev 20100227-1433 Perlin noise function
#pragma once
#include "noise_generator.h"
#include "numeric_structs.h"

/**
 * @brief A faithful reimplementation of the Infdev and Beta perlin noise generator
 * 
 */
class NoisePerlin : public NoiseGenerator {
protected:
	int32_t permutations[512];
	Vec3 coordinate;
	double GenerateNoiseBase(Vec3 _pos);
	void InitPermTable(Java::Random& _rand);

public:
	NoisePerlin();
	NoisePerlin(Java::Random& _rand);
	double GenerateNoise(Vec2 _coord);
	double GenerateNoise(Vec3 _coord);
	void GenerateNoise(std::vector<double>& _noiseField, Vec3 _offset, Int3 _size, Vec3 _scale, double _amplitude);
};