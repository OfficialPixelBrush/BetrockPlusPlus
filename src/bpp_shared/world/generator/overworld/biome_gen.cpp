/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "biome_gen.h"
#include "chunk.h"

/**
 * @brief Construct a new Beta 1.7.3 Biome
 * 
 * @param seed The world seed that the biome-generator will use
 */
BiomeGenerator::BiomeGenerator(int64_t _seed) {
	// Init Biome Noise
	Java::Random randTemp = Java::Random(_seed * 9871L);
	Java::Random randHum = Java::Random(_seed * 39811L);
	Java::Random randWeird = Java::Random(_seed * 543321L);
	temperatureNoiseGen = NoiseOctavesSimplex(randTemp, 4);
	humidityNoiseGen = NoiseOctavesSimplex(randHum, 4);
	weirdnessNoiseGen = NoiseOctavesSimplex(randWeird, 2);
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
void BiomeGenerator::GenerateBiomeMap(Biome _biomeMap[], std::vector<double>& _temperature, std::vector<double>& _humidity,
                                      std::vector<double>& _weirdness, Int2 _blockPos) {
	// Get noise values
	static constexpr Int32_2 maxArea{ CHUNK_WIDTH, CHUNK_WIDTH };
	this->temperatureNoiseGen.GenerateOctaves(_temperature, _blockPos, maxArea, Vec2{ double(0.025f), double(0.025f) },
	                                            0.25);
	this->humidityNoiseGen.GenerateOctaves(_humidity, _blockPos, maxArea, Vec2{ double(0.05f), double(0.05f) },
	                                         1.0 / 3.0);
	this->weirdnessNoiseGen.GenerateOctaves(_weirdness, _blockPos, maxArea, Vec2{ 0.25, 0.25 }, 0.5882352941176471);
	size_t index = 0;

	// Iterate over each block column
	for (int32_t iX = 0; iX < CHUNK_WIDTH; ++iX) {
		for (int32_t iZ = 0; iZ < CHUNK_WIDTH; ++iZ) {
			double weird = _weirdness[index] * 1.1 + 0.5;
			double scale = 0.01;
			double limit = 1.0 - scale;
			double temp = (_temperature[index] * 0.15 + 0.7) * limit + weird * scale;
			scale = 0.002;
			limit = 1.0 - scale;
			double humi = (_humidity[index] * 0.15 + 0.5) * limit + weird * scale;
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
			_temperature[index] = temp;
			_humidity[index] = humi;
			// Get the biome from the lookup
			_biomeMap[index] = GetBiomeFromLookup(float(temp), float(humi));
			index++;
		}
	}
}

Biome BiomeGenerator::GetBiomeAtPoint(Int2 _worldPos) {
	std::vector<double> temp(1), humi(1), weird(1);

	this->temperatureNoiseGen.GenerateOctaves(temp, Int2{ _worldPos.x, _worldPos.y }, Int32_2{ 1, 1 },
	                                            Vec2{ double(0.025f), double(0.025f) }, 0.25);
	this->humidityNoiseGen.GenerateOctaves(humi, Int2{ _worldPos.x, _worldPos.y }, Int32_2{ 1, 1 },
	                                         Vec2{ double(0.05f), double(0.05f) }, 1.0 / 3.0);
	this->weirdnessNoiseGen.GenerateOctaves(weird, Int2{ _worldPos.x, _worldPos.y }, Int32_2{ 1, 1 },
	                                          Vec2{ 0.25, 0.25 }, 0.5882352941176471);

	double w = weird[0] * 1.1 + 0.5;
	double t = (temp[0] * 0.15 + 0.7) * 0.99 + w * 0.01;
	double h = (humi[0] * 0.15 + 0.5) * 0.998 + w * 0.002;
	t = 1.0 - (1.0 - t) * (1.0 - t);
	if (t < 0.0)
		t = 0.0;
	if (t > 1.0)
		t = 1.0;
	if (h < 0.0)
		h = 0.0;
	if (h > 1.0)
		h = 1.0;

	return GetBiomeFromLookup(float(t), float(h));
}

/**
 * @brief Generates the temperature map values
 * 
 * @param temperature The temperature map that'll be used/written to during generation
 * @param weirdness The weirdness map that'll be used/written to during generation
 * @param blockPos The x,z block-space coordindate of the chunk
 * @param max The size of the area that'll be generated (16x16 by default)
 */
void BiomeGenerator::GenerateTemperature(std::vector<double>& _temperature, std::vector<double>& _weirdness,
                                         Int2 _blockPos, Int2 _max) {
	if (_temperature.empty() || _temperature.size() < size_t(_max.x * _max.y)) {
		_temperature.resize(size_t(_max.x * _max.y), 0.0);
	}

	this->temperatureNoiseGen.GenerateOctaves(_temperature, _blockPos, _max, Vec2{ double(0.025f), double(0.025f) },
	                                            0.25);
	this->weirdnessNoiseGen.GenerateOctaves(_weirdness, _blockPos, _max, Vec2{ 0.25, 0.25 }, 0.5882352941176471);
	size_t index = 0;

	// Iterate over each block column
	for (int32_t x = 0; x < _max.x; ++x) {
		for (int32_t z = 0; z < _max.y; ++z) {
			double weird = _weirdness[index] * 1.1 + 0.5;
			double scale = 0.01;
			double limit = 1.0 - scale;
			double temp = (_temperature[index] * 0.15 + 0.7) * limit + weird * scale;
			temp = 1.0 - (1.0 - temp) * (1.0 - temp);
			// Limit values to 0.0 - 1.0
			if (temp < 0.0)
				temp = 0.0;
			if (temp > 1.0)
				temp = 1.0;
			// Write the temperature values back
			_temperature[index] = temp;
			++index;
		}
	}
}