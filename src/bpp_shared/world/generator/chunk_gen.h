/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

//#include "cave_gen.h"
//#include "feature_gen.h"
//#include "tree_gen.h"
#include "biome_gen.h"
//#include "blockHelper.h"
#include "world.h"

/**
 * @brief A faithful reimplementation of the Beta 1.7.3 world generator
 * 
 */
class Generator {
  private:
	Java::Random rand;
	// Perlin Noise Generators
	NoiseOctaves<NoisePerlin> lowNoiseGen;
	NoiseOctaves<NoisePerlin> highNoiseGen;
	NoiseOctaves<NoisePerlin> selectorNoiseGen;
	NoiseOctaves<NoisePerlin> sandGravelNoiseGen;
	NoiseOctaves<NoisePerlin> stoneNoiseGen;
	NoiseOctaves<NoisePerlin> continentalnessNoiseGen;
	NoiseOctaves<NoisePerlin> depthNoiseGen;
	NoiseOctaves<NoisePerlin> treeDensityNoiseGen;

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
	std::vector<Biome> biomeMap;
	std::vector<double> temperature;
	std::vector<double> humidity;
	std::vector<double> weirdness;

	// Cave Gen
	//std::unique_ptr<Beta173Caver> caver;

	int64_t seed = 0;

	void GenerateTerrain(Chunk& chunk);
	void GenerateTerrainNoise(std::vector<double> &terrainMap, Int3 cpos, Int3 max);
	void ReplaceBlocksForBiome(Chunk& chunk);
	Biome GetBiomeAt(Int2 worldPos);

  public:
	Generator(int64_t seed);
	~Generator() = default;
	void GenerateChunk(Chunk& chunk);
	bool PopulateChunk(Chunk& chunk);
};