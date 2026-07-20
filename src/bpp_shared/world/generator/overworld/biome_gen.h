/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once
#include "biomes.h"
#include "noise_octaves_simplex.h"

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
	BiomeGenerator(int64_t _seed);
	Biome GetBiomeAtPoint(Int2 _worldPos);
	void GenerateBiomeMap(Biome _biomeMap[], std::vector<double>& _temperature, std::vector<double>& _humidity,
	                      std::vector<double>& _weirdness, Int2 _blockPos);
	void GenerateTemperature(std::vector<double>& _temperature, std::vector<double>& _weirdness, Int2 _chunkPos, Int2 _max);
};