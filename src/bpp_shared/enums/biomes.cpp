/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#include "biomes.h"

/**
 * @brief Get the Top Block object
 * 
 * @param biome The biome to get the top/surface block of
 * @return The top/surface block BlockType
 */
BlockType GetTopBlock(Biome biome) {
	if (biome == BIOME_DESERT || biome == BIOME_ICEDESERT) {
		return BLOCK_SAND;
	}
	return BLOCK_GRASS;
}


/**
 * @brief Get the Filler Block object
 * 
 * @param biome The biome to get the filler block of
 * @return The filler block BlockType
 */
BlockType GetFillerBlock(Biome biome) {
	if (biome == BIOME_DESERT || biome == BIOME_ICEDESERT) {
		return BLOCK_SAND;
	}
	return BLOCK_DIRT;
}

/**
 * @brief Get the correct biome based on the passed temperature and humidity values
 * 
 * @param temperature Temperature value
 * @param humidity Humidity/Downfall value
 * @return The appropriate Biome for the passed values
 */
Biome GetBiome(float temperature, float humidity) {
	humidity *= temperature;
	if (temperature < 0.1f) {
		return BIOME_TUNDRA;
	}
	if (humidity < 0.2f) {
		if (temperature < 0.5f) {
			return BIOME_TUNDRA;
		}
		if (temperature < 0.95f) {
			return BIOME_SAVANNA;
		}
		return BIOME_DESERT;
	}
	if (humidity > 0.5f && temperature < 0.7f) {
		return BIOME_SWAMPLAND;
	}
	if (temperature < 0.5f) {
		return BIOME_TAIGA;
	}
	if (temperature < 0.97f) {
		if (humidity < 0.35f) {
			return BIOME_SHRUBLAND;
		}
		return BIOME_FOREST;
	}
	if (humidity < 0.45f) {
		return BIOME_PLAINS;
	}
	if (humidity < 0.9f) {
		return BIOME_SEASONALFOREST;
	}
	return BIOME_RAINFOREST;
}

/**
 * @brief Gets the appropriate biome from the Biome LUT
 * 
 * @param temperature Temperature value
 * @param humidity Humidity/Downfall value
 * @return The appropriate Biome for the passed values
 */
Biome GetBiomeFromLookup(float temperature, float humidity) {
	int32_t temp = int32_t(temperature * 63.0f);
	int32_t humi = int32_t(humidity * 63.0f);
	return BiomeLUT[size_t(temp + humi * 64)];
}