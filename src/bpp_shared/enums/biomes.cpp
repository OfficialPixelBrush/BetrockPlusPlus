#include "biomes.h"

BlockType GetTopBlock(Biome biome) {
	if (biome == BIOME_DESERT || biome == BIOME_ICEDESERT) {
		return BLOCK_SAND;
	}
	return BLOCK_GRASS;
}

BlockType GetFillerBlock(Biome biome) {
	if (biome == BIOME_DESERT || biome == BIOME_ICEDESERT) {
		return BLOCK_SAND;
	}
	return BLOCK_DIRT;
}

Biome BiomeLUT[64 * 64];

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

void GenerateBiomeLookup() {
	for (int32_t temp = 0; temp < 64; ++temp) {
		for (int32_t humi = 0; humi < 64; ++humi) {
			BiomeLUT[temp + humi * 64] = GetBiome(float(temp) / 63.0f, float(humi) / 63.0f);
		}
	}
}

Biome GetBiomeFromLookup(float temperature, float humidity) {
	int32_t temp = int32_t(temperature * 63.0f);
	int32_t humi = int32_t(humidity * 63.0f);
	return BiomeLUT[temp + humi * 64];
}