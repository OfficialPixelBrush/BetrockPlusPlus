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
#include <memory>
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

	thread_local std::vector<uint8_t> raw;
	raw.assign(size_t(total), 0);
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

	const int columnStride = CHUNK_WIDTH * CHUNK_WIDTH;
	int i = 0;
	for (int x = _xmin; x < _xmax; x++) {
		for (int z = _zmin; z < _zmax; z++) {
			const int col = z * CHUNK_WIDTH + x;
			for (int y = _ymin; y < _ymax; y++, i++) {
				const int idx = y * columnStride + col;
				blockData[i] = uint8_t(_chunk.blocks[idx]);
				const uint8_t metaByte = _chunk.nibbleBlockMeta[idx >> 1];
				const uint8_t meta = (idx & 1) ? uint8_t(metaByte >> 4) : uint8_t(metaByte & 0x0F);
				const uint8_t light = _chunk.lightNibble[idx];
				packNibble(metaData[i >> 1], meta, i & 1);
				packNibble(blockLight[i >> 1], uint8_t(light & 0x0F), i & 1);
				packNibble(skyLight[i >> 1], uint8_t(light >> 4), i & 1);
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
	thread_local std::vector<uint8_t> compressed;
	compressed.resize(maxSize);
	const size_t actualSize = libdeflate_zlib_compress(compressor.get(), raw.data(), static_cast<size_t>(total),
	                                                   compressed.data(), maxSize);
	if (actualSize == 0)
		return {};
	return { compressed.data(), compressed.data() + actualSize };
}
} // namespace ChunkSerializer