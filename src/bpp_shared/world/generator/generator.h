/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once
#include "generator.h"
#include <cstdint>
#include "./overworld/feature_gen.h"

/**
 * @brief The base generator class
 * 
 */
class Generator {
  protected:
	Java::Random rand;
	int64_t seed = 0;
  public:
	Generator(int64_t seed);
	virtual ~Generator() = default;
	virtual void GenerateChunk(Chunk& chunk);
	virtual bool PopulateChunk(Chunk& chunk, WorldWrapper& world);
};