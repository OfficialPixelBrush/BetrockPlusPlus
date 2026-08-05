/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once
#include "biomes.h"
#include "noise_octaves_simplex.h"
#include <span>

/**
 * @brief A faithful reimplementation of the Beta 1.7.3 biome generator
 * 
 */
class BiomeGenerator {
private:
	// Simplex Noise Generators
	NoiseOctavesSimplex temperatureNoiseGen;
	NoiseOctavesSimplex humidityNoiseGen;
	NoiseOctavesSimplex weirdnessNoiseGen;

public:
	BiomeGenerator();
	BiomeGenerator(int64_t _seed);
	Biome GetBiomeAtPoint(Int2 _worldPos);
	void GenerateBiomeMap(Biome _biomeMap[], std::span<double> _temperature, std::span<double> _humidity,
	                      std::span<double> _weirdness, Int2 _blockPos);
	void GenerateTemperature(std::span<double> _temperature, std::span<double> _weirdness, Int2 _chunkPos, Int2 _max);
};
