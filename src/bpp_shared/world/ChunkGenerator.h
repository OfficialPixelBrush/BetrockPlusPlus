/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

namespace ChunkGenerator {
	void generate(Chunk& chunk, int64_t seed) {
		for (int x = 0; x < 16; x++)
			for (int z = 0; z < 16; z++) {
				float normalized = (((std::sin((x + (chunk.pos.x * 16)) / 16.0) + 1.0f) / 2.0f) + ((std::cos((z + (chunk.pos.z * 16)) / 16.0) + 1.0f) / 2.0f)) / 2.0f;
				int top_block = (int)(normalized * 8);

				for (int y = 0; y < top_block; y++) {
					chunk.setBlock({ x, y, z }, BlockType::BLOCK_STONE);
				}
				chunk.setBlock({ x, top_block, z }, BlockType::BLOCK_GRASS);
			}
	}
}