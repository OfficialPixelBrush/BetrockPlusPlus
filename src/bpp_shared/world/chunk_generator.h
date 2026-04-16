/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include <cmath>
#include "noise/noise_octaves.h"

/*
NoiseOctaves<NoisePerlin> perlin(16);

namespace ChunkGenerator {
	void generate(Chunk& chunk, [[maybe_unused]] int64_t seed) {
		for (int x = 0; x < CHUNK_WIDTH; x++)
			for (int z = 0; z < CHUNK_WIDTH; z++) {
				float normalized = float(perlin.GenerateOctaves(Vec2 {
					double(x + (chunk.cpos.x * CHUNK_WIDTH))*5.0,
					double(z + (chunk.cpos.z * CHUNK_WIDTH))*5.0
				}));
				normalized /= 6500;
				normalized *= 10;
				std::cout << normalized << std::endl;
				//normalized += 50;
				float normalized = (((
						std::sin(float(x + (chunk.cpos.x * CHUNK_WIDTH)) / float(CHUNK_WIDTH)) + 1.0f
					) / 2.0f) + ((
						std::cos(float(z + (chunk.cpos.z * CHUNK_WIDTH)) / float(CHUNK_WIDTH)) + 1.0f
					) / 2.0f
				)) / 2.0f;
				int top_block = int(normalized * 8);

				for (int y = 0; y < top_block; y++) {
					chunk.setBlock({ x, y, z }, BlockType::BLOCK_STONE);
				}
				chunk.setBlock({ x, top_block, z }, BlockType::BLOCK_GRASS);
			}
	}
}
*/