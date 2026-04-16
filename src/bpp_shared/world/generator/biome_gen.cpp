/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "biome_gen.h"

/**
 * @brief Construct a new Beta 1.7.3 Biome
 * 
 * @param seed The world seed that the biome-generator will use
 */
BiomeGenerator::BiomeGenerator(int64_t seed) {
    // Init Biome Noise
    Java::Random randTemp = Java::Random(seed * 9871L);
    Java::Random randHum = Java::Random(seed * 39811L);
    Java::Random randWeird = Java::Random(seed * 543321L);
    temperatureNoiseGen = NoiseOctaves<NoiseSimplex>(randTemp, 4);
    humidityNoiseGen = NoiseOctaves<NoiseSimplex>(randHum, 4);
    weirdnessNoiseGen = NoiseOctaves<NoiseSimplex>(randWeird, 2);
}

/**
 * @brief Generate Biomes based on simplex noise and updates the temperature, humidity and weirdness maps
 * 
 * @param biomeMap The biome map the final Biome values should be written to
 * @param temperature The temperature map that'll be used/written to during generation
 * @param humidity The humidity map that'll be used/written to during generation
 * @param weirdness The weirdness map that'll be used/written to during generation
 * @param blockPos The x,z block-space coordindate of the chunk
 * @param max The size of the area that'll be generated (16x16 by default)
 */
void BiomeGenerator::GenerateBiomeMap(std::vector<Biome>& biomeMap, std::vector<double>& temperature, std::vector<double>& humidity, std::vector<double>& weirdness, Int2 blockPos, Int2 max) {
	// Init Biome map
	if (biomeMap.empty() || int32_t(biomeMap.size()) < max.x * max.y) {
		biomeMap.resize(size_t(max.x * max.y), BIOME_NONE);
	}

	// Get noise values
	// We found an oversight in the original code! max.z is NEVER used for getting the noise range!
	// Although this is irrelevant for all intents and purposes, as max.x always equals max.z
	Int32_2 max_x = Int32_2{max.x, max.x};
	this->temperatureNoiseGen.GenerateOctaves(temperature, blockPos, max_x, Vec2{double(0.025f), double(0.025f)}, 0.25);
	this->humidityNoiseGen.GenerateOctaves(humidity, blockPos, max_x, Vec2{double(0.05f), double(0.05f)}, 1.0 / 3.0);
	this->weirdnessNoiseGen.GenerateOctaves(weirdness, blockPos, max_x, Vec2{0.25, 0.25}, 0.5882352941176471);
	size_t index = 0;

	// Iterate over each block column
	for (int32_t iX = 0; iX < max.x; ++iX) {
		for (int32_t iZ = 0; iZ < max.y; ++iZ) {
			double weird = weirdness[index] * 1.1 + 0.5;
			double scale = 0.01;
			double limit = 1.0 - scale;
			double temp = (temperature[index] * 0.15 + 0.7) * limit + weird * scale;
			scale = 0.002;
			limit = 1.0 - scale;
			double humi = (humidity[index] * 0.15 + 0.5) * limit + weird * scale;
			temp = 1.0 - (1.0 - temp) * (1.0 - temp);
			// Limit values to 0.0 - 1.0
			if (temp < 0.0)
				temp = 0.0;
			if (humi < 0.0)
				humi = 0.0;
			if (temp > 1.0)
				temp = 1.0;
			if (humi > 1.0)
				humi = 1.0;

			// Write the temperature and humidity values back
			temperature[index] = temp;
			humidity[index] = humi;
			// Get the biome from the lookup
			biomeMap[index] = GetBiomeFromLookup(float(temp), float(humi));
			index++;
		}
	}
}

/**
 * @brief Generates the temperature map values
 * 
 * @param temperature The temperature map that'll be used/written to during generation
 * @param weirdness The weirdness map that'll be used/written to during generation
 * @param blockPos The x,z block-space coordindate of the chunk
 * @param max The size of the area that'll be generated (16x16 by default)
 */
void BiomeGenerator::GenerateTemperature(std::vector<double>& temperature, std::vector<double>& weirdness, Int2 blockPos, Int2 max) {
	if (temperature.empty() || temperature.size() < size_t(max.x * max.y)) {
		temperature.resize(size_t(max.x * max.y), 0.0);
	}

	this->temperatureNoiseGen.GenerateOctaves(temperature, blockPos, max, Vec2{double(0.025f), double(0.025f)}, 0.25);
	this->weirdnessNoiseGen.GenerateOctaves(weirdness, blockPos, max, Vec2{0.25, 0.25}, 0.5882352941176471);
	size_t index = 0;

	for (int32_t x = 0; x < max.x; ++x) {
		for (int32_t z = 0; z < max.y; ++z) {
			double var9 = weirdness[index] * 1.1 + 0.5;
			double scale = 0.01;
			double limit = 1.0 - scale;
			double temp = (temperature[index] * 0.15 + 0.7) * limit + var9 * scale;
			temp = 1.0 - (1.0 - temp) * (1.0 - temp);
			if (temp < 0.0)
				temp = 0.0;

			if (temp > 1.0)
				temp = 1.0;

			temperature[index] = temp;
			++index;
		}
	}
}