/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once
#include "../generator.h"
#include "../noise/noise_octaves_perlin.h"
#include "../shared/cave_gen.h"
#include "../shared/feature_gen.h"
#include "biomes.h"
#include "generator/overworld/biome_gen.h"

/**
 * @brief Seed-initialized, read-only octave/permutation tables for the overworld.
 * Shared across generation threads; scratch buffers stay per-thread on OverworldGenerator.
 */
struct OverworldNoise {
	explicit OverworldNoise(int64_t _seed);

	int64_t seed = 0;
	NoiseOctavesPerlin lowNoiseGen;
	NoiseOctavesPerlin highNoiseGen;
	NoiseOctavesPerlin selectorNoiseGen;
	NoiseOctavesPerlin sandGravelNoiseGen;
	NoiseOctavesPerlin stoneNoiseGen;
	NoiseOctavesPerlin continentalnessNoiseGen;
	NoiseOctavesPerlin depthNoiseGen;
	NoiseOctavesPerlin treeDensityNoiseGen;
	BiomeGenerator biomeGen;
};

/**
 * @brief A faithful reimplementation of the Beta 1.7.3 Overworld Generator
 */
class OverworldGenerator : public Generator {
private:
	OverworldNoise* tables = nullptr;

	// Stored noise Fields
	static constexpr Int3 MAX{ CHUNK_WIDTH / 4 + 1, CHUNK_HEIGHT / 8 + 1, CHUNK_WIDTH / 4 + 1 };
	static constexpr size_t MAX_AREA = (MAX.x * MAX.z);
	static constexpr size_t MAX_VOLUME = (MAX.x * MAX.y * MAX.z);
	double terrainNoiseField[MAX_VOLUME];
	double lowNoiseField[MAX_VOLUME];
	double highNoiseField[MAX_VOLUME];
	double selectorNoiseField[MAX_VOLUME];
	double continentalnessNoiseField[MAX_AREA];
	double depthNoiseField[MAX_AREA];

	double sandNoise[CHUNK_WIDTH * CHUNK_WIDTH];
	double gravelNoise[CHUNK_WIDTH * CHUNK_WIDTH];
	double stoneNoise[CHUNK_WIDTH * CHUNK_WIDTH];

	Biome biomeMap[CHUNK_AREA];
	double temperature[CHUNK_AREA];
	double humidity[CHUNK_AREA];
	double weirdness[CHUNK_AREA];

	CaveGenerator caver;

	void GenerateTerrain(Chunk& _chunk);
	void GenerateTerrainNoise(Int3 _cpos, Int3 _max);
	void ReplaceBlocksForBiome(Chunk& _chunk);
	Biome GetBiomeAt(Int2 _worldPos);
	void GenerateTreeForBiome(WorldWrapper& _world, Java::Random& _rand, Int3 _pos, Biome _biome);

public:
	OverworldGenerator() = default;
	explicit OverworldGenerator(OverworldNoise& _tables) {
		Bind(_tables);
	}
	~OverworldGenerator() = default;

	void Bind(OverworldNoise& _tables) {
		tables = &_tables;
		seed = _tables.seed;
	}

	void GenerateChunk(Chunk& _chunk) override;
	bool PopulateChunk(Chunk& _chunk, WorldWrapper& _world) override;
};
