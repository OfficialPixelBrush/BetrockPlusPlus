/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once

#include "java_random.h"
#include "world.h"
#include "blockHelper.h"

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
	bool GenerateLake(World *world, Java::Random& rand, Int3 pos);
	bool GenerateDungeon(World *world, Java::Random& rand, Int3 pos);
	Item GenerateDungeonChestLoot(Java::Random& rand);
	std::string PickMobToSpawn(Java::Random& rand);
	bool GenerateClay(World *world, Java::Random& rand, Int3 pos, int32_t numberOfBlocks = 0);
	bool GenerateMinable(World *world, Java::Random& rand, Int3 pos, int32_t numberOfBlocks = 0);
	bool GenerateFlowers(World *world, Java::Random& rand, Int3 pos);
	bool GenerateTallgrass(World *world, Java::Random& rand, Int3 pos);
	bool GenerateDeadbush(World *world, Java::Random& rand, Int3 pos);
	bool GenerateSugarcane(World *world, Java::Random& rand, Int3 pos);
	bool GeneratePumpkins(World *world, Java::Random& rand, Int3 pos);
	bool GenerateCacti(World *world, Java::Random& rand, Int3 pos);
	bool GenerateLiquid(World *world, Java::Random& rand, Int3 pos);
};