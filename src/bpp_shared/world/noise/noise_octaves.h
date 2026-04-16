/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once

#include "java_random.h"
#include "noise_generator.h"
#include "noise_perlin.h"
#include "noise_simplex.h"
#include <vector>

template <typename T> class NoiseOctaves {
  public:
	NoiseOctaves() {} // This should never be used!
	NoiseOctaves(int32_t octaves);
	NoiseOctaves(Java::Random& rand, int32_t octaves);
	// Used by infdev
	double GenerateOctaves(Vec3 offset);
	// Used by Perlin
	double GenerateOctaves(Vec2 offset);
	void GenerateOctaves(std::vector<double> &noiseField, Vec3 coordinate, Int32_3 size, Vec3 p_scale);
	void GenerateOctaves(std::vector<double> &noiseField, Int32_2 offset, Int32_2 size, Vec2 scale, [[maybe_unused]] double unused);
	// Used by Simplex
	void GenerateOctaves(std::vector<double> &noiseField, Vec2 offset, Int32_2 size, Vec2 scale, double var12);
	void GenerateOctaves(std::vector<double> &noiseField, double var2, double var4, int32_t var6, int32_t scale, double var8, double var10, double var12, double var14);

  private:
	int32_t octaves;
	std::vector<T> generator_collection;
};

template <typename T> NoiseOctaves<T>::NoiseOctaves(int32_t poctaves) : octaves(poctaves) {
	this->octaves = poctaves;
	for (int32_t i = 0; i < this->octaves; ++i) {
		// It will make its own Java::Random object
		generator_collection.push_back(T());
	}
}

template <typename T> NoiseOctaves<T>::NoiseOctaves(Java::Random& rand, int32_t poctaves) {
	this->octaves = poctaves;
	for (int32_t i = 0; i < this->octaves; ++i) {
		generator_collection.push_back(T(rand));
	}
}

// Only used by infdev
template <typename T> double NoiseOctaves<T>::GenerateOctaves(Vec3 offset) {
	double value = 0.0;
	double scale = 1.0;

	for (int32_t i = 0; i < this->octaves; ++i) {
		value += this->generator_collection[i].GenerateNoise(offset * scale) / scale;
		scale /= 2.0;
	}

	return value;
}

// Used my Perlin
// func_647_a
template <typename T> double NoiseOctaves<T>::GenerateOctaves(Vec2 offset) {
	double value = 0.0;
	double scale = 1.0;

	for (size_t i = 0; i < size_t(this->octaves); ++i) {
		value += this->generator_collection[i].GenerateNoise(offset * scale) / scale;
		scale /= 2.0;
	}

	return value;
}

// Used by Beta 1.7.3
// GenerateNoiseOctaves
template <typename T>
void NoiseOctaves<T>::GenerateOctaves(std::vector<double> &noiseField, Vec3 coordinate, Int32_3 size, Vec3 p_scale) {
	if (noiseField.empty()) {
		noiseField.resize(size_t(size.x * size.y * size.z), 0.0);
	} else {
		for (size_t i = 0; i < noiseField.size(); ++i) {
			noiseField[i] = 0.0;
		}
	}

	double scale = 1.0;

	for (int32_t octave = 0; octave < this->octaves; ++octave) {
		this->generator_collection[octave]->GenerateNoise(noiseField, coordinate, size, p_scale * scale, scale);
		scale /= 2.0;
	}
}

// func_4103_a
template <typename T>
void NoiseOctaves<T>::GenerateOctaves(std::vector<double> &noiseField, Int32_2 offset, Int32_2 size, Vec2 scale, [[maybe_unused]] double unused) {
	this->GenerateOctaves(noiseField, double(offset.x), 10.0, double(offset.z), size.x, 1, size.z, scale.x, 1.0, scale.z);
}

// Comes from simplex Octaves
template <typename T>
void NoiseOctaves<T>::GenerateOctaves(std::vector<double> &noiseField, Vec2 offset, Int32_2 size, Vec2 scale, double var12) {
	this->GenerateOctaves(noiseField, offset.x, offset.z, size.x, size.z, scale.x, scale.z, var12, 0.5);
}

template <typename T>
void NoiseOctaves<T>::GenerateOctaves(std::vector<double> &noiseField, double var2, double var4, int32_t var6, int32_t scale,
									  double var8, double var10, double var12, double var14) {
	var8 /= 1.5;
	var10 /= 1.5;
	if (!noiseField.empty() && int32_t(noiseField.size()) >= var6 * scale) {
		for (size_t i = 0; i < noiseField.size(); ++i) {
			noiseField[i] = 0.0;
		}
	} else {
		noiseField.resize(size_t(var6 * scale), 0.0);
	}

	double var21 = 1.0;
	double var18 = 1.0;

	for (int32_t octave = 0; octave < this->octaves; ++octave) {
		this->generator_collection[octave]->GenerateNoise(noiseField, var2, var4, var6, scale, var8 * var18,
														 var10 * var18, 0.55 / var21);
		var18 *= var12;
		var21 *= var14;
	}
}