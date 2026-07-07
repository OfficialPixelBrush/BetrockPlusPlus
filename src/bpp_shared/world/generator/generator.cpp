/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "generator.h"

Generator::Generator(int64_t seed) : m_seed(seed) {}

void Generator::GenerateChunk([[maybe_unused]] Chunk& chunk) {}

bool Generator::PopulateChunk([[maybe_unused]] Chunk& chunk, [[maybe_unused]] WorldWrapper& world) {
	return true;
}