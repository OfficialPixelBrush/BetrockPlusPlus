/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include "constants.h"
#include "enums/blocks.h"
#include "helpers/packed_array.h"
#include "numeric_structs.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

// A 16x16x128 chunk that's fully allocated as a set of plain arrays (~80KB).
// Terrain generation and population read/write this directly, since there are no layers to allocate
// or counts to keep in sync. Once a chunk has settled it gets compacted into SubChunks, see
// Chunk::ConvertToSubChunks().
//
// The layout is deliberately the same as SubChunk's, just continued upwards:
//   index = (y << 8) | (z << 4) | x
// That makes slice s occupy [s * SUB_CHUNK_VOLUME, (s + 1) * SUB_CHUNK_VOLUME), with the exact same
// local indices SubChunk::LocalIndex() gives, so converting is a straight copy per slice.
//
// The index-based accessors mirror SubChunk's, so code can treat both storages the same way.
struct FlatChunk {
	static_assert(CHUNK_WIDTH == 16 && SUB_CHUNK_SIZE == 16, "FlatChunk::Index assumes 16x16 columns");
	static constexpr int VOLUME = CHUNK_VOLUME;
	static constexpr int SLICE_VOLUME = SUB_CHUNK_VOLUME;
	static_assert(SLICE_VOLUME % 2 == 0, "Nibble slices must start on a byte boundary");

	using NibbleArray = PackedArray<VOLUME, 4>;

	std::array<BlockType, VOLUME> block;
	NibbleArray meta;
	NibbleArray blockLight;
	NibbleArray skyLight;

	// Same defaults as an empty SubChunk: air, no meta, no block light, full sky light
	FlatChunk() {
		Reset();
	}

	// Takes chunk-relative coordinates, y is 0..CHUNK_HEIGHT-1
	static constexpr int Index(Int3 _pos) {
		return (_pos.y << 8) | (_pos.z << 4) | _pos.x;
	}

	// Reads. Take an index, see Index()
	inline BlockType GetBlock(int _idx) const {
		return block[size_t(_idx)];
	}
	inline uint8_t GetMeta(int _idx) const {
		return meta.Get(size_t(_idx));
	}
	inline uint8_t GetBlockLight(int _idx) const {
		return blockLight.Get(size_t(_idx));
	}
	inline uint8_t GetSkyLight(int _idx) const {
		return skyLight.Get(size_t(_idx));
	}

	// Writes
	inline void SetBlock(int _idx, BlockType _val) {
		block[size_t(_idx)] = _val;
	}
	inline void SetMeta(int _idx, uint8_t _val) {
		meta.Set(size_t(_idx), _val);
	}
	inline void SetBlockLight(int _idx, uint8_t _val) {
		blockLight.Set(size_t(_idx), _val);
	}
	inline void SetSkyLight(int _idx, uint8_t _val) {
		skyLight.Set(size_t(_idx), _val);
	}

	// Overwrites a contiguous run of blocks
	inline void FillBlockSpan(size_t _start, size_t _count, BlockType _type) {
		std::fill_n(block.begin() + std::ptrdiff_t(_start), _count, _type);
	}
	// Overwrites a contiguous run of nibbles in any of the three nibble arrays
	static void FillNibbleSpan(NibbleArray& _arr, size_t _start, size_t _count, uint8_t _val) {
		_val &= 0x0Fu;
		const size_t end = _start + _count;
		// An odd start or end shares a byte with a neighbour we must not touch
		if ((_start & 1) && _start < end)
			_arr.Set(_start++, _val);
		if ((end & 1) && _start < end)
			_arr.Set(end - 1, _val);
		const size_t last = end & ~size_t(1);
		if (_start < last)
			std::memset(_arr.data.data() + _start / 2, int(_val | (_val << 4)), (last - _start) / 2);
	}

	// Number of voxels of this type. O(volume), SubChunk keeps counts but this doesn't
	inline int CountBlock(BlockType _type) const {
		return int(std::count(block.begin(), block.end(), _type));
	}
	inline bool HasBlock(BlockType _type) const {
		return std::find(block.begin(), block.end(), _type) != block.end();
	}
	// Appends the chunk-local position of every voxel of this type, lowest index first
	// (the same order Chunk::FindBlocks gives for SubChunks). Returns how many were added.
	inline int FindBlocks(BlockType _type, std::vector<Int3>& _out) const {
		int found = 0;
		for (int i = 0; i < VOLUME; i++) {
			if (block[size_t(i)] != _type)
				continue;
			_out.push_back({ i & 15, i >> 8, (i >> 4) & 15 });
			found++;
		}
		return found;
	}

	// Back to all-default: air, no meta, no block light, full sky light
	inline void Reset() {
		block.fill(BLOCK_AIR);
		meta.data.fill(0);
		blockLight.data.fill(0);
		skyLight.data.fill(0xFF);
	}
};
