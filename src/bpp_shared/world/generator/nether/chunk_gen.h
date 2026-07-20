/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "../../noise/noise_octaves_perlin.h"
#include "../generator.h"
#include "../shared/cave_gen.h"

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
	double terrainNoiseField[(CHUNK_WIDTH / 4 + 1) * (CHUNK_HEIGHT / 8 + 1) * (CHUNK_WIDTH / 4 + 1)];
	std::vector<double> lowNoiseField;
	std::vector<double> highNoiseField;
	std::vector<double> selectorNoiseField;
	std::vector<double> continentalnessNoiseField;
	std::vector<double> depthNoiseField;

	std::vector<double> sandNoise;
	std::vector<double> gravelNoise;
	std::vector<double> stoneNoise;

	// Cave Gen
	CaveGenerator caver;

	void GenerateTerrain(Chunk& _chunk);
	void GenerateTerrainNoise(Int3 _cpos, Int3 _max);
	void ReplaceBlocksForBiome(Chunk& _chunk);

public:
	NetherGenerator(int64_t _seed);
	~NetherGenerator() = default;
	void GenerateChunk(Chunk& _chunk) override;
	bool PopulateChunk(Chunk& _chunk, WorldWrapper& _world) override;
};