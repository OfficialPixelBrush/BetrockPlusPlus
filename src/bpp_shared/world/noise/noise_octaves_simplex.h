/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once

#include "java_math.h"
#include "java_random.h"
#include "noise_simplex.h"
#include <vector>

class NoiseOctavesSimplex {
  public:
	NoiseOctavesSimplex() {}
	NoiseOctavesSimplex(int32_t octaves);
	NoiseOctavesSimplex(Java::Random& rand, int32_t octaves);

	// func_4112_a
	void GenerateOctaves(std::vector<double>& noiseField, Vec2 offset, Int32_2 size, Vec2 scale, double lacunarity);
	void GenerateOctaves(std::vector<double>& noiseField, Int32_2 offset, Int32_2 size, Vec2 scale, double lacunarity);
	// func_4111_a
	void GenerateOctaves(std::vector<double>& noiseField, Vec2 offset, Int32_2 size, Vec2 scale, double lacunarity, double persistence);

  private:
	int32_t octaves;
	std::vector<NoiseSimplex> generator_collection;
};

inline NoiseOctavesSimplex::NoiseOctavesSimplex(int32_t poctaves) : octaves(poctaves) {
	for (int32_t i = 0; i < octaves; ++i)
		generator_collection.push_back(NoiseSimplex());
}

inline NoiseOctavesSimplex::NoiseOctavesSimplex(Java::Random& rand, int32_t poctaves) : octaves(poctaves) {
	for (int32_t i = 0; i < octaves; ++i)
		generator_collection.push_back(NoiseSimplex(rand));
}

inline void NoiseOctavesSimplex::GenerateOctaves(std::vector<double>& noiseField, Int32_2 offset, Int32_2 size, Vec2 scale, double lacunarity) {
	this->GenerateOctaves(noiseField, Int32_2{Java::DoubleToInt32(offset.x), Java::DoubleToInt32(offset.y)}, size, scale, lacunarity);
}

inline void NoiseOctavesSimplex::GenerateOctaves(std::vector<double>& noiseField, Vec2 offset, Int32_2 size, Vec2 scale, double lacunarity) {
	this->GenerateOctaves(noiseField, offset, size, scale, lacunarity, 0.5);
}

inline void NoiseOctavesSimplex::GenerateOctaves(std::vector<double>& noiseField, Vec2 offset, Int32_2 size, Vec2 scale, double lacunarity, double persistence) {
	scale.x /= 1.5;
	scale.y /= 1.5;
	if (!noiseField.empty() && int32_t(noiseField.size()) >= size.x * size.y) {
		for (size_t i = 0; i < noiseField.size(); ++i)
			noiseField[i] = 0.0;
	} else {
		noiseField.resize(size_t(size.x * size.y), 0.0);
	}

	double frequency = 1.0;
	double amplitude = 1.0;
	for (size_t octave = 0; octave < size_t(octaves); ++octave) {
		generator_collection[octave].GenerateNoise(noiseField, offset, size, scale * amplitude, 0.55 / frequency);
		amplitude *= lacunarity;
		frequency *= persistence;
	}
}