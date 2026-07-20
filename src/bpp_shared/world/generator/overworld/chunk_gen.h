/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "../generator.h"
#include "../noise/noise_octaves_perlin.h"
#include "../shared/cave_gen.h"
#include "../shared/feature_gen.h"
#include "biomes.h"

/**
 * @brief A faithful reimplementation of the Beta 1.7.3 Overworld Generator
 * 
 */
class OverworldGenerator : public Generator {
private:
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
	double terrainNoiseField[(CHUNK_WIDTH / 4 + 1) * (CHUNK_HEIGHT / 8 + 1) * (CHUNK_WIDTH / 4 + 1)];
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

	void GenerateTerrain(Chunk& _chunk);
	void GenerateTerrainNoise(Int3 _cpos, Int3 _max);
	void ReplaceBlocksForBiome(Chunk& _chunk);
	Biome GetBiomeAt(Int2 _worldPos);
	void GenerateTreeForBiome(WorldWrapper& _world, Java::Random& _rand, Int3 _pos, Biome _biome);

public:
	OverworldGenerator(int64_t _seed);
	~OverworldGenerator() = default;
	void GenerateChunk(Chunk& _chunk) override;
	bool PopulateChunk(Chunk& _chunk, WorldWrapper& _world) override;
};