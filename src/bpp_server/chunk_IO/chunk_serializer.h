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
inline std::vector<uint8_t> Serialize(const Chunk& _chunk, int _xmin = 0, int _xmax = 16, int _ymin = 0,
                                      int _ymax = CHUNK_HEIGHT, int _zmin = 0, int _zmax = 16) {
	int sizeX = _xmax - _xmin;
	int sizeY = _ymax - _ymin;
	int sizeZ = _zmax - _zmin;

	int blocks = sizeX * sizeY * sizeZ;
	int nibbles = (blocks + 1) / 2;
	int total = blocks + nibbles * 3;

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
			for (int y = _ymin; y < _ymax; y++, i++) {
				Int3 pos{ x, y, z };
				blockData[i] = uint8_t(_chunk.GetBlock(pos));
				packNibble(metaData[i >> 1], _chunk.GetMeta(pos), i & 1);
				packNibble(blockLight[i >> 1], _chunk.GetBlockLight(pos), i & 1);
				packNibble(skyLight[i >> 1], _chunk.GetSkyLight(pos), i & 1);
			}
		}
	}

	libdeflate_compressor* compressor = libdeflate_alloc_compressor(6);
	if (!compressor)
		return {};
	size_t maxSize = libdeflate_zlib_compress_bound(compressor, static_cast<size_t>(total));
	std::vector<uint8_t> compressed(maxSize);
	size_t actualSize = libdeflate_zlib_compress(compressor, raw.data(), static_cast<size_t>(total), compressed.data(),
	                                             maxSize);
	libdeflate_free_compressor(compressor);
	compressed.resize(actualSize);
	return compressed;
}
} // namespace ChunkSerializer