/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "../generator.h"
#include "world.h"
#include "../../noise/noise_octaves_perlin.h"

/**
 * @brief A faithful reimplementation of the Beta 1.7.3 Nether Generator
 * 
 */
class NetherGenerator : public Generator {
  private:
	// Perlin Noise Generators
	NoiseOctavesPerlin lowNoiseGen;
	NoiseOctavesPerlin highNoiseGen;
	NoiseOctavesPerlin selectorNoiseGen;
	NoiseOctavesPerlin sandGravelNoiseGen;
	NoiseOctavesPerlin stoneNoiseGen;
	NoiseOctavesPerlin continentalnessNoiseGen;
	NoiseOctavesPerlin depthNoiseGen;

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

	// Cave Gen
	//CaveGenerator caver;

	void GenerateTerrain(Chunk& chunk);
	void GenerateTerrainNoise(std::vector<double> &terrainMap, Int3 cpos, Int3 max);
	void ReplaceBlocksForBiome(Chunk& chunk);
  public:
	NetherGenerator(int64_t seed);
	~NetherGenerator() = default;
	void GenerateChunk(Chunk& chunk) override;
	bool PopulateChunk(Chunk& chunk, WorldWrapper& world) override;
};