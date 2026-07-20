/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "generator.h"

Generator::Generator(int64_t _seed) : seed(_seed) {}

void Generator::GenerateChunk([[maybe_unused]] Chunk& _chunk) {}

bool Generator::PopulateChunk([[maybe_unused]] Chunk& _chunk, [[maybe_unused]] WorldWrapper& _world) {
	return true;
}