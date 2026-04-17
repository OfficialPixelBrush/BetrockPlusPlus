/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "biomes.h"
#include "biome_gen.h"
#include "cave_gen.h"
// #include "feature_gen.h"
// #include "tree_gen.h"
#include "world.h"

/**
 * @brief A faithful reimplementation of the Beta 1.7.3 world generator
 * 
 */
class Generator {
  private:
	Java::Random rand;
	// Perlin Noise Generators
	NoiseOctavesPerlin lowNoiseGen;
	NoiseOctavesPerlin highNoiseGen;
	NoiseOctavesPerlin selectorNoiseGen;
	NoiseOctavesPerlin sandGravelNoiseGen;
	NoiseOctavesPerlin stoneNoiseGen;
	NoiseOctavesPerlin continentalnessNoiseGen;
	NoiseOctavesPerlin depthNoiseGen;
	NoiseOctavesPerlin treeDensityNoiseGen;

	// Stored noise Fields
	std::vector<double> terrainNoiseField;
	std::vector<double> lowNoiseField;
	std::vector<double> highNoiseField;
	std::vector<double> selectorNoiseField;
	std::vector<double> continentalnessNoiseField;
	std::vector<double> depthNoiseField;

	std::vector<double> sandNoise;
	std::vector<double> gravelNoise;
	std::vector<double> stoneNoise;

	// Biome Vectors
	Biome biomeMap[CHUNK_AREA];
	std::vector<double> temperature;
	std::vector<double> humidity;
	std::vector<double> weirdness;

	// Cave Gen
	CaveGenerator caver;

	int64_t seed = 0;

	void GenerateTerrain(Chunk& chunk);
	void GenerateTerrainNoise(std::vector<double> &terrainMap, Int3 cpos, Int3 max);
	void ReplaceBlocksForBiome(Chunk& chunk);
	Biome GetBiomeAt(Int2 worldPos);

  public:
	Generator(int64_t seed);
	~Generator() = default;
	void GenerateChunk(Chunk& chunk);
	bool PopulateChunk([[maybe_unused]] Chunk& chunk, [[maybe_unused]] WorldManager world);
};