/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "chunk.h"
#include <libdeflate.h>
#include <vector>

namespace ChunkSerializer {
inline std::vector<uint8_t> Serialize(const Chunk& _chunk, int _xmin = 0, int _xmax = CHUNK_WIDTH, int _ymin = 0,
                                      int _ymax = CHUNK_HEIGHT, int _zmin = 0, int _zmax = CHUNK_WIDTH) {
	const int sizeX = _xmax - _xmin;
	const int sizeY = _ymax - _ymin;
	const int sizeZ = _zmax - _zmin;

	const int blocks = sizeX * sizeY * sizeZ;
	const int nibbles = (blocks + 1) / 2;
	const int total = blocks + nibbles * 3;

	std::vector<uint8_t> raw(size_t(total), 0);
	uint8_t* blockData = raw.data();
	uint8_t* metaData = blockData + blocks;
	uint8_t* blockLight = metaData + nibbles;
	uint8_t* skyLight = blockLight + nibbles;

	auto packNibble = [](uint8_t& _byte, uint8_t _val, bool _high) {
		if (_high)
			_byte = uint8_t((_byte & 0x0F) | ((_val & 0x0F) << 4));
		else
			_byte = uint8_t((_byte & 0xF0) | (_val & 0x0F));
	};

	int i = 0;
	for (int x = _xmin; x < _xmax; x++) {
		for (int z = _zmin; z < _zmax; z++) {
			for (int y = _ymin; y < _ymax;) {
				const int slabEnd = CrossPlatform::Math::Min(_ymax, (y | (SUB_CHUNK_SIZE - 1)) + 1);
				const SubChunk* sub = _chunk.GetSubChunk(y);
				if (!sub) {
					const SubChunkFill& fill = _chunk.subChunkFill[size_t(y >> 4)];
					for (; y < slabEnd; y++, i++) {
						blockData[i] = uint8_t(fill.type);
						// Meta is always 0 for an empty slab, and the buffer is zero-initialised
						packNibble(blockLight[i >> 1], fill.blockLight, i & 1);
						packNibble(skyLight[i >> 1], fill.skyLight, i & 1);
					}
				} else {
					for (; y < slabEnd; y++, i++) {
						const int idx = SubChunk::LocalIndex({ x, y, z });
						const uint8_t light = sub->light[idx];
						const uint8_t metaByte = sub->nibbleBlockMeta[idx >> 1];
						blockData[i] = uint8_t(sub->blocks[idx]);
						packNibble(metaData[i >> 1], (idx & 1) ? Chunk::GetNibbleHigh(metaByte) : Chunk::GetNibbleLow(metaByte), i & 1);
						packNibble(blockLight[i >> 1], Chunk::GetNibbleLow(light), i & 1);
						packNibble(skyLight[i >> 1], Chunk::GetNibbleHigh(light), i & 1);
					}
				}
			}
		}
	}

	thread_local std::unique_ptr<libdeflate_compressor, decltype(&libdeflate_free_compressor)> compressor(
	    nullptr, libdeflate_free_compressor);
	if (!compressor)
		compressor.reset(libdeflate_alloc_compressor(1));
	if (!compressor)
		return {};
	size_t maxSize = libdeflate_zlib_compress_bound(compressor.get(), static_cast<size_t>(total));
	std::vector<uint8_t> compressed(maxSize);
	const size_t actualSize = libdeflate_zlib_compress(compressor.get(), raw.data(), static_cast<size_t>(total),
	                                                   compressed.data(), maxSize);
	if (actualSize == 0)
		return {};
	compressed.resize(actualSize);
	return compressed;
}
} // namespace ChunkSerializer