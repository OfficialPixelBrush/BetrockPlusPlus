/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "../generator.h"
#include "biomes.h"
#include "biome_gen.h"
#include "cave_gen.h"
#include "feature_gen.h"
#include "tree_gen.h"
#include "world.h"

/**
 * @brief A faithful reimplementation of the Beta 1.7.3 Overworld Generator
 * 
 */
class OverworldGenerator : public Generator {
  private:
	// Perlin Noise Generators
	NoiseOctavesPerlin m_lowNoiseGen;
	NoiseOctavesPerlin m_highNoiseGen;
	NoiseOctavesPerlin m_selectorNoiseGen;
	NoiseOctavesPerlin m_sandGravelNoiseGen;
	NoiseOctavesPerlin m_stoneNoiseGen;
	NoiseOctavesPerlin m_continentalnessNoiseGen;
	NoiseOctavesPerlin m_depthNoiseGen;
	NoiseOctavesPerlin m_treeDensityNoiseGen;

	// Stored noise Fields
	std::vector<double> m_terrainNoiseField;
	std::vector<double> m_lowNoiseField;
	std::vector<double> m_highNoiseField;
	std::vector<double> m_selectorNoiseField;
	std::vector<double> m_continentalnessNoiseField;
	std::vector<double> m_depthNoiseField;

	std::vector<double> m_sandNoise;
	std::vector<double> m_gravelNoise;
	std::vector<double> m_stoneNoise;

	// Biome Vectors
	Biome m_biomeMap[CHUNK_AREA];
	std::vector<double> m_temperature;
	std::vector<double> m_humidity;
	std::vector<double> m_weirdness;

	// Cave Gen
	CaveGenerator m_caver;

	void GenerateTerrain(Chunk& chunk);
	void GenerateTerrainNoise(std::vector<double> &terrainMap, Int3 cpos, Int3 max);
	void ReplaceBlocksForBiome(Chunk& chunk);
	Biome GetBiomeAt(Int2 worldPos);
	void GenerateTreeForBiome(WorldWrapper& world, Java::Random& rand, Int3 pos, Biome biome);

  public:
	OverworldGenerator(int64_t seed);
	~OverworldGenerator() = default;
	void GenerateChunk(Chunk& chunk) override;
	bool PopulateChunk(Chunk& chunk, WorldWrapper& world) override;
};