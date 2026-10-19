/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "generator.h"

Generator::Generator(int64_t _seed) : seed(_seed) {}

void Generator::GenerateChunk(Chunk& /*_chunk*/) {}

bool Generator::PopulateChunk(Chunk& /*_chunk*/, WorldWrapper& /*_world*/) {
	return true;
}