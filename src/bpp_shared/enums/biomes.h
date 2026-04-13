#pragma once
#include "blocks.h"

enum Biome {
	BIOME_NONE = 0,
	BIOME_RAINFOREST = 1,
	BIOME_SWAMPLAND = 2,
	BIOME_SEASONALFOREST = 3,
	BIOME_FOREST = 4,
	BIOME_SAVANNA = 5,
	BIOME_SHRUBLAND = 6,
	BIOME_TAIGA = 7,
	BIOME_DESERT = 8,
	BIOME_PLAINS = 9,
	BIOME_ICEDESERT = 10,
	BIOME_TUNDRA = 11,
	BIOME_HELL = 12,
	BIOME_SKY = 13
};

Biome GetBiome(float temperature, float humidity);
void GenerateBiomeLookup();
Biome GetBiomeFromLookup(float temperature, float humidity);

BlockType GetTopBlock(Biome biome);
BlockType GetFillerBlock(Biome biome);