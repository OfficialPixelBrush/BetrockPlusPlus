/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once
#include "biomes.h"
#include "noise_octaves.h"
#include "noise_simplex.h"

/**
 * @brief A faithful reimplementation of the Beta 1.7.3 biome generator
 * 
 */
class BiomeGenerator {
    private:
        // Simplex Noise Generators
        NoiseOctaves<NoiseSimplex> temperatureNoiseGen;
        NoiseOctaves<NoiseSimplex> humidityNoiseGen;
        NoiseOctaves<NoiseSimplex> weirdnessNoiseGen;
    public:
        BiomeGenerator(int64_t seed);
        void GenerateBiomeMap(std::vector<Biome>& biomeMap, std::vector<double>& temperature, std::vector<double>& humidity, std::vector<double>& weirdness, Int2 blockPos, Int2 max);
	    void GenerateTemperature(std::vector<double>& temperature, std::vector<double>& weirdness, Int2 chunkPos, Int2 max);
};