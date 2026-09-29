/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "blocks/block_properties.h"
#include "constants.h"
#include "enums/biomes.h"
#include "helpers/cross_platform.h"
#include "helpers/packed_array.h"
#include "nbt/nbt.h"
#include "tile_entities/tile_entity.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <numeric_structs.h>

enum class ChunkState : uint8_t {
	Unloaded,
	Generating,
	Loading,
	Generated,
	Populating,
	Populated,
	Unloading
};

// A 16x16x16 slice of a Chunk. Chunks are stored as a vertical stack of these, and a slice is only
// allocated once it holds something worth storing
struct SubChunk {
	static constexpr int SIZE = SUB_CHUNK_SIZE;
	static constexpr int VOLUME = SIZE * SIZE * SIZE;
	static constexpr int META_VOLUME = VOLUME / 2;

	BlockType blocks[VOLUME] = { BLOCK_AIR };
	uint8_t lightNibble[VOLUME] = { 0 };
	uint8_t nibbleBlockMeta[META_VOLUME] = { 0 };

	// Takes chunk-relative coordinates, only the low 4 bits of y are used.
	static inline int LocalIndex(Int3 _pos) {
		return ((_pos.y & (SIZE - 1)) * SIZE * SIZE) + (_pos.z * SIZE) + _pos.x;
	}

	inline void Clear() {
		std::memset(blocks, 0, sizeof(blocks));
		std::memset(lightNibble, 0, sizeof(lightNibble));
		std::memset(nibbleBlockMeta, 0, sizeof(nibbleBlockMeta));
	}

	// Sets the sky light (high nibble) of every block
	inline void FillSkyLight(uint8_t _val) {
		const uint8_t hi = uint8_t((_val & 0x0Fu) << 4);
		for (uint8_t& b : lightNibble)
			b = uint8_t((b & 0x0Fu) | hi);
	}

	// Sets the block light (low nibble) of every block
	inline void FillBlockLight(uint8_t _val) {
		const uint8_t lo = uint8_t(_val & 0x0Fu);
		for (uint8_t& b : lightNibble)
			b = uint8_t((b & 0xF0u) | lo);
	}
};

struct CombinedLight {
	uint8_t blockLight : 4 = 0;
	uint8_t skyLight : 4 = 0;
};

struct Chunk {
	static constexpr int VOLUME = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;
	static_assert(CHUNK_HEIGHT % SUB_CHUNK_SIZE == 0, "CHUNK_HEIGHT must be a multiple of SUB_CHUNK_SIZE");
	static_assert(SUB_CHUNK_SIZE == CHUNK_WIDTH, "SubChunk indexing assumes SUB_CHUNK_SIZE == CHUNK_WIDTH");

	Int32_2 cpos;
	std::atomic_bool inUse{ false };

	// Block, light and metadata storage, bottom to top. A slot is null until it needs to hold data
	std::array<std::unique_ptr<SubChunk>, SUB_CHUNK_COUNT> subChunks;
	std::array<CombinedLight, SUB_CHUNK_COUNT> subChunkFillLight = {};
	// What block the sub-chunk is filled with. Defaults to air.
	std::array<BlockType, SUB_CHUNK_COUNT> subChunkFillBlock = {};

	std::atomic<ChunkState> state{ ChunkState::Unloaded };
	uint8_t heightMap[CHUNK_AREA] = {};
	float temperature[CHUNK_AREA] = {};
	float humidity[CHUNK_AREA] = {};
	PackedArray<CHUNK_AREA, 4> biomes;

	bool isTerrainPopulated : 1 = false;
	bool isModified : 1 = false;
	bool spawnChunk : 1 = false;
	bool refreshLighting : 1 = false;
	bool climateBaked : 1 = false;

	// Tile entities
	std::vector<std::shared_ptr<TileEntity>> tileEntities;

	// Used for loading entities into the world from disk
	std::vector<Tag> entityTags;

	// Sub-chunk containing chunk-relative height _y, or nullptr if it isn't allocated
	inline SubChunk* GetSubChunk(int _y) {
		return subChunks[size_t(_y >> 4)].get();
	}
	inline const SubChunk* GetSubChunk(int _y) const {
		return subChunks[size_t(_y >> 4)].get();
	}
	// Allocates slot _index (if needed) holding what a null slot reads as
	SubChunk& CreateSubChunk(int _index);

	inline uint8_t SetNibble(uint8_t _hi, uint8_t _lo) const {
		return uint8_t(((_hi & 0x0Fu) << 4) | (_lo & 0x0Fu));
	}
	inline uint8_t GetNibbleLow(uint8_t _byte) const {
		return _byte & 0x0Fu;
	}
	inline uint8_t GetNibbleHigh(uint8_t _byte) const {
		return (_byte >> 4) & 0x0Fu;
	}
	inline float GetTemperature(Int2 _pos) const {
		return temperature[(_pos.x << 4) | _pos.y];
	}
	inline float GetHumidity(Int2 _pos) const {
		return humidity[(_pos.x << 4) | _pos.y];
	}
	inline uint8_t GetHeightValue(Int2 _pos) const {
		return heightMap[(_pos.y << 4) | _pos.x];
	}
	inline void SetHeightValue(Int2 _pos, uint8_t _val) {
		heightMap[(_pos.y << 4) | _pos.x] = _val;
	}
	inline BlockType GetBlock(Int3 _pos) const {
		const SubChunk* sub = GetSubChunk(_pos.y);
		return sub ? sub->blocks[SubChunk::LocalIndex(_pos)] : subChunkFillBlock[size_t(_pos.y >> 4)];
	}
	// Doesn't flag as modified
	inline void SetBlockRaw(Int3 _pos, BlockType _id) {
		SubChunk* sub = GetSubChunk(_pos.y);
		if (!sub) {
			if (_id == BLOCK_AIR)
				return; // Writing air into nothing, nothing to do
			sub = &CreateSubChunk(_pos.y >> 4);
		}
		sub->blocks[SubChunk::LocalIndex(_pos)] = _id;
	}
	inline void SetBlock(Int3 _pos, BlockType _id) {
		SetBlockRaw(_pos, _id);
		isModified = true;
	}
	inline uint8_t GetMeta(Int3 _pos) const {
		const SubChunk* sub = GetSubChunk(_pos.y);
		if (!sub)
			return 0;
		int idx = SubChunk::LocalIndex(_pos);
		uint8_t byte = sub->nibbleBlockMeta[idx >> 1];
		return (idx & 1) ? GetNibbleHigh(byte) : GetNibbleLow(byte);
	}
	inline void SetMeta(Int3 _pos, uint8_t _meta) {
		SubChunk* sub = GetSubChunk(_pos.y);
		if (!sub) {
			isModified = true;
			if ((_meta & 0x0Fu) == 0)
				return;
			sub = &CreateSubChunk(_pos.y >> 4);
		}
		int idx = SubChunk::LocalIndex(_pos);
		uint8_t& byte = sub->nibbleBlockMeta[idx >> 1];
		byte = (idx & 1) ? SetNibble(_meta, GetNibbleLow(byte)) : SetNibble(GetNibbleHigh(byte), _meta);
		isModified = true;
	}
	inline uint8_t GetBlockLight(Int3 _pos) const {
		const SubChunk* sub = GetSubChunk(_pos.y);
		return sub ? GetNibbleLow(sub->lightNibble[SubChunk::LocalIndex(_pos)]) : subChunkFillLight[size_t(_pos.y >> 4)].blockLight;
	}
	inline uint8_t GetSkyLight(Int3 _pos) const {
		const SubChunk* sub = GetSubChunk(_pos.y);
		return sub ? GetNibbleHigh(sub->lightNibble[SubChunk::LocalIndex(_pos)]) : subChunkFillLight[size_t(_pos.y >> 4)].skyLight;
	}
	inline void SetBlockLight(Int3 _pos, uint8_t _val) {
		SubChunk* sub = GetSubChunk(_pos.y);
		if (!sub) {
			isModified = true;
			if ((_val & 0x0Fu) == 0)
				return;
			sub = &CreateSubChunk(_pos.y >> 4);
		}
		uint8_t& byte = sub->lightNibble[SubChunk::LocalIndex(_pos)];
		byte = SetNibble(GetNibbleHigh(byte), _val);
		isModified = true;
	}
	inline void SetSkyLight(Int3 _pos, uint8_t _val) {
		SubChunk* sub = GetSubChunk(_pos.y);
		if (!sub) {
			isModified = true;
			if ((_val & 0x0Fu) == subChunkFillLight[size_t(_pos.y >> 4)].skyLight)
				return;
			sub = &CreateSubChunk(_pos.y >> 4);
		}
		uint8_t& byte = sub->lightNibble[SubChunk::LocalIndex(_pos)];
		byte = SetNibble(_val, GetNibbleLow(byte));
		isModified = true;
	}
	inline int GetBlockLightValue(Int3 _pos, int _skySubtracted) const {
		int sky = CrossPlatform::Math::Max(0, int(GetSkyLight(_pos)) - _skySubtracted);
		int block = int(GetBlockLight(_pos));
		return CrossPlatform::Math::Min(15, CrossPlatform::Math::Max(sky, block));
	}

	int GetHighestPoint() const;
	bool CanBlockSeeSky(Int3 _pos) const;
	void GenerateHeightMap();
	void GenerateHeightMapColumn(Int2 _pos);
	void GenerateSkylightMap();
	void RelightColumn(Int2 _pos);
	void Clear();

	// Frees a slot if it holds nothing but air, no meta, no block light and a single sky light value.
	// Returns true if it was freed. Compact() does this for every slot.
	bool CompactSubChunk(int _index);
	void Compact();
	// Deep copies blocks, light and meta
	void CopyStorageFrom(const Chunk& _other);

	inline int AllocatedSubChunks() const {
		int n = 0;
		for (const auto& sub : subChunks)
			n += sub ? 1 : 0;
		return n;
	}
	// Approximate heap + inline footprint. Prefer this over sizeof(Chunk) now that storage is lazy.
	inline size_t GetMemoryUsage() const {
		return sizeof(Chunk) + size_t(AllocatedSubChunks()) * sizeof(SubChunk);
	}
};
