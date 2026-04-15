/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include <cmath>

namespace ChunkGenerator {
	void generate(Chunk& chunk, [[maybe_unused]] int64_t seed) {
		for (int x = 0; x < CHUNK_WIDTH; x++)
			for (int z = 0; z < CHUNK_WIDTH; z++) {
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