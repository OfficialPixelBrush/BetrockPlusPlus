/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once

#include "java_random.h"
#include "world.h"
#include "base_structs.h"

/**
 * @brief Beta 1.7.3 Feature Generators
 * This class wraps up all the features that Beta 1.7.3 can generate
 * into a single class for ease of access.
 */
class FeatureGenerator {
  private:
	BlockType type = BLOCK_AIR;
	int8_t meta = 0;

  public:
	FeatureGenerator() {};
	FeatureGenerator(BlockType type);
	FeatureGenerator(BlockType type, int8_t meta);
	bool GenerateLake(WorldManager& world, Java::Random& rand, Int3 pos);
	bool GenerateDungeon(WorldManager& world, Java::Random& rand, Int3 pos);
	Item GenerateDungeonChestLoot(Java::Random& rand);
	std::string PickMobToSpawn(Java::Random& rand);
	bool GenerateClay(WorldManager& world, Java::Random& rand, Int3 pos, int32_t numberOfBlocks = 0);
	bool GenerateMinable(WorldManager& world, Java::Random& rand, Int3 pos, int32_t numberOfBlocks = 0);
	bool GenerateFlowers(WorldManager& world, Java::Random& rand, Int3 pos);
	bool GenerateTallgrass(WorldManager& world, Java::Random& rand, Int3 pos);
	bool GenerateDeadbush(WorldManager& world, Java::Random& rand, Int3 pos);
	bool GenerateSugarcane(WorldManager& world, Java::Random& rand, Int3 pos);
	bool GeneratePumpkins(WorldManager& world, Java::Random& rand, Int3 pos);
	bool GenerateCacti(WorldManager& world, Java::Random& rand, Int3 pos);
	bool GenerateLiquid(WorldManager& world, Java::Random& rand, Int3 pos);
};