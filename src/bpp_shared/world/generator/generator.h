/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once
#include "./shared/feature_gen.h"
#include <cstdint>

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
	Generator() = default;
	virtual ~Generator() = default;
	virtual void GenerateChunk(Chunk& chunk);
	virtual bool PopulateChunk(Chunk& chunk, WorldWrapper& world);
};