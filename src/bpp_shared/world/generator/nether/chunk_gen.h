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
	NoiseOctavesPerlin m_lowNoiseGen;
	NoiseOctavesPerlin m_highNoiseGen;
	NoiseOctavesPerlin m_selectorNoiseGen;
	NoiseOctavesPerlin m_sandGravelNoiseGen;
	NoiseOctavesPerlin m_stoneNoiseGen;
	NoiseOctavesPerlin m_continentalnessNoiseGen;
	NoiseOctavesPerlin m_depthNoiseGen;

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