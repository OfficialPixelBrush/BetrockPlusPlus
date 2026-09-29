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
#include <cassert>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <numeric_structs.h>

// -- An explanation for those that want it, on why the chunks are laid out this way --
// Chunks are much more memory efficient in B++ by utilizing
// some optimizations that're common in modern voxel engines.
// One of these is either called sub-chunks or palettes.
// Sub-chunks is less confusing for this situation, so we'll call it that here.
// 
// Simply put, a normal chunk is 16x128x16, and contains 4 kinds of data:
// The Block type, the metadata value, the block light and sky light.
// Separately, these take up 81920 Bytes, since even air with no metadata is stored as is. With 1k chunks, that's ~80MB!
// We can reduce this significantly by not storing data we don't need. This is done via sub-chunks.
//
// Each chunk holds 8 16x16x16 volumes, which're only allocated when they contain non-homogenous data.
// For example, most blocks above y=64 are air,
// so we don't need to store those 4K volumes above that level that're just air.
// Instead we can store a single byte that represents that sub-chunk that says "Yup, this whole thing is air."
// We can do the same with the other kinds of data, such as metadata and lighting,
// which results in significant savings (~2/3 on average)
// 
// This already reduces memory usage significantly, but we can do better.
// If we handle each kind of data separately, via layers,
// we can further reduce the amount of excess usage.
// For example, an underground, fully stone sub-chunk, with a single iron ore block in it,
// can still have it's sky and blocklight represented in the compressed
// format, and doesn't need to be fully stored as raw data.
//
// The final layout becomes:
// Chunk (16x128x16 b,m,bl,sl)
//  -> Sub-Chunk (16x16x16 b,m,bl,sl) 
//    -> Layer (Block Type)
//    -> Layer (Metadata)
//    -> Layer (Block Light)
//    -> Layer (Sky Light)
//
// The only downside of the layer approach is that now
// most of those allocations are rather small, at most 4K for the block layer,
// and only 2K for the meta, block light and skylight layers.
// This can result in more significant memory fragmentation, though there are
// several kinds of data we use that are bigger contributors to this.
//
// Kind regards, Pixel Brush

enum class ChunkState : uint8_t {
	Unloaded,
	Generating,
	Loading,
	Generated,
	Populating,
	Populated,
	Unloading
};

// A 16x16x16 slice of a Chunk. Chunks are stored as a vertical stack of these.
struct SubChunk {
	static constexpr int SIZE = SUB_CHUNK_SIZE;
	static constexpr int VOLUME = SIZE * SIZE * SIZE;
	static constexpr int NIBBLE_BYTES = VOLUME / 2;

	using BlockLayer = std::array<BlockType, VOLUME>;
	using NibbleLayer = std::array<uint8_t, NIBBLE_BYTES>;

	// Null = uniform, see the matching fill below
	std::unique_ptr<BlockLayer> blocks;
	std::unique_ptr<NibbleLayer> meta;
	std::unique_ptr<NibbleLayer> blockLight;
	std::unique_ptr<NibbleLayer> skyLight;

	// What a null layer reads as. Defaults to air, no meta, no block light and full sky light.
	BlockType blockFill = BLOCK_AIR;
	uint8_t metaFill = 0;
	uint8_t blockLightFill = 0;
	uint8_t skyLightFill = 15;

	// Takes chunk-relative coordinates, only the low 4 bits of y are used.
	static inline int LocalIndex(Int3 _pos) {
		return ((_pos.y & (SIZE - 1)) * SIZE * SIZE) + (_pos.z * SIZE) + _pos.x;
	}

	static inline uint8_t GetNibble(const NibbleLayer& _layer, int _idx) {
		const uint8_t byte = _layer[size_t(_idx >> 1)];
		return (_idx & 1) ? uint8_t(byte >> 4) : uint8_t(byte & 0x0Fu);
	}
	static inline void SetNibble(NibbleLayer& _layer, int _idx, uint8_t _val) {
		uint8_t& byte = _layer[size_t(_idx >> 1)];
		byte = (_idx & 1) ? uint8_t((byte & 0x0Fu) | ((_val & 0x0Fu) << 4)) : uint8_t((byte & 0xF0u) | (_val & 0x0Fu));
	}

	// Reads. Take a local index, see LocalIndex()
	inline BlockType GetBlock(int _idx) const {
		return blocks ? (*blocks)[size_t(_idx)] : blockFill;
	}
	inline uint8_t GetMeta(int _idx) const {
		return meta ? GetNibble(*meta, _idx) : metaFill;
	}
	inline uint8_t GetBlockLight(int _idx) const {
		return blockLight ? GetNibble(*blockLight, _idx) : blockLightFill;
	}
	inline uint8_t GetSkyLight(int _idx) const {
		return skyLight ? GetNibble(*skyLight, _idx) : skyLightFill;
	}

	// Writes. A layer is only allocated when the value differs from what it currently reads as
	inline void SetBlock(int _idx, BlockType _val) {
		if (!blocks) {
			if (_val == blockFill)
				return;
			AllocBlocks();
		}
		(*blocks)[size_t(_idx)] = _val;
	}
	inline void SetMeta(int _idx, uint8_t _val) {
		_val &= 0x0Fu;
		if (!meta) {
			if (_val == metaFill)
				return;
			AllocMeta();
		}
		SetNibble(*meta, _idx, _val);
	}
	inline void SetBlockLight(int _idx, uint8_t _val) {
		_val &= 0x0Fu;
		if (!blockLight) {
			if (_val == blockLightFill)
				return;
			AllocBlockLight();
		}
		SetNibble(*blockLight, _idx, _val);
	}
	inline void SetSkyLight(int _idx, uint8_t _val) {
		_val &= 0x0Fu;
		if (!skyLight) {
			if (_val == skyLightFill)
				return;
			AllocSkyLight();
		}
		SetNibble(*skyLight, _idx, _val);
	}

	// Sets every voxel in the slice, releasing that layer's storage
	inline void FillBlocks(BlockType _val) {
		blocks.reset();
		blockFill = _val;
	}
	inline void FillMeta(uint8_t _val) {
		meta.reset();
		metaFill = _val & 0x0Fu;
	}
	inline void FillBlockLight(uint8_t _val) {
		blockLight.reset();
		blockLightFill = _val & 0x0Fu;
	}
	inline void FillSkyLight(uint8_t _val) {
		skyLight.reset();
		skyLightFill = _val & 0x0Fu;
	}

	// Allocates a layer holding its current fill value in every voxel. Only meant for null layers
	void AllocBlocks();
	void AllocMeta();
	void AllocBlockLight();
	void AllocSkyLight();

	// Frees a layer if every voxel in it holds the same value, moving that value into the fill.
	// Returns true if the layer was freed.
	bool CompactBlocks();
	bool CompactMeta();
	bool CompactBlockLight();
	bool CompactSkyLight();
	// Tries every layer, each on its own. Returns true if at least one was freed.
	bool Compact();

	// Back to all-default: air, no meta, no block light, full sky light. Frees every layer
	void Reset();
	// Deep copies every layer
	void CopyFrom(const SubChunk& _other);

	// Bytes of layer storage currently allocated, not counting sizeof(SubChunk) itself
	inline size_t GetHeapUsage() const {
		return (blocks ? sizeof(BlockLayer) : 0) + (meta ? sizeof(NibbleLayer) : 0) +
		       (blockLight ? sizeof(NibbleLayer) : 0) + (skyLight ? sizeof(NibbleLayer) : 0);
	}
	inline bool HasAnyLayer() const {
		return blocks || meta || blockLight || skyLight;
	}
};

// TODO: Check viability of "full chunks", for faster access.
// Basically how stuff worked before SubChunks,
// for those that prefer that system for the performance gain.
// Plus we could fall back to those if all 8 subchunks are filled anyways.
struct Chunk {
	static constexpr int VOLUME = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;
	static_assert(CHUNK_HEIGHT % SUB_CHUNK_SIZE == 0, "CHUNK_HEIGHT must be a multiple of SUB_CHUNK_SIZE");
	static_assert(SUB_CHUNK_SIZE == CHUNK_WIDTH, "SubChunk indexing assumes SUB_CHUNK_SIZE == CHUNK_WIDTH");

	Int32_2 cpos;
	std::atomic_bool inUse{ false };

	// Block, light and metadata storage, bottom to top. Every slice is always present, but the layers
	// inside it (blocks, meta, block light, sky light) are only allocated once they need to hold data.
	std::array<SubChunk, SUB_CHUNK_COUNT> subChunks;

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

	static constexpr bool InBounds(int _y) {
		return _y >= 0 && _y < CHUNK_HEIGHT;
	}

	// Normal subchunk return, null if _y is out of bounds
	inline SubChunk* GetSubChunk(int _y) {
		if (InBounds(_y))
			return &subChunks[size_t(_y >> 4)];
		return nullptr;
	}
	// Const subchunk return
	inline const SubChunk* GetSubChunk(int _y) const {
		if (InBounds(_y))
			return &subChunks[size_t(_y >> 4)];
		return nullptr;
	}

	static inline uint8_t SetNibble(uint8_t _hi, uint8_t _lo) {
		return uint8_t(((_hi & 0x0Fu) << 4) | (_lo & 0x0Fu));
	}
	static inline uint8_t GetNibbleLow(uint8_t _byte) {
		return _byte & 0x0Fu;
	}
	static inline uint8_t GetNibbleHigh(uint8_t _byte) {
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
		if (!InBounds(_pos.y))
			return BLOCK_AIR;
		return subChunks[size_t(_pos.y >> 4)].GetBlock(SubChunk::LocalIndex(_pos));
	}
	// Doesn't flag as modified
	inline void SetBlockRaw(Int3 _pos, BlockType _id) {
		if (!InBounds(_pos.y))
			return;
		subChunks[size_t(_pos.y >> 4)].SetBlock(SubChunk::LocalIndex(_pos), _id);
	}
	inline void SetBlock(Int3 _pos, BlockType _id) {
		SetBlockRaw(_pos, _id);
		isModified = true;
	}
	inline uint8_t GetMeta(Int3 _pos) const {
		if (!InBounds(_pos.y))
			return 0;
		return subChunks[size_t(_pos.y >> 4)].GetMeta(SubChunk::LocalIndex(_pos));
	}
	inline void SetMeta(Int3 _pos, uint8_t _meta) {
		if (!InBounds(_pos.y))
			return;
		subChunks[size_t(_pos.y >> 4)].SetMeta(SubChunk::LocalIndex(_pos), _meta);
		isModified = true;
	}
	inline uint8_t GetBlockLight(Int3 _pos) const {
		if (!InBounds(_pos.y))
			return 0;
		return subChunks[size_t(_pos.y >> 4)].GetBlockLight(SubChunk::LocalIndex(_pos));
	}
	inline uint8_t GetSkyLight(Int3 _pos) const {
		if (!InBounds(_pos.y))
			return 0;
		return subChunks[size_t(_pos.y >> 4)].GetSkyLight(SubChunk::LocalIndex(_pos));
	}
	inline void SetBlockLight(Int3 _pos, uint8_t _val) {
		if (!InBounds(_pos.y))
			return;
		subChunks[size_t(_pos.y >> 4)].SetBlockLight(SubChunk::LocalIndex(_pos), _val);
		isModified = true;
	}
	inline void SetSkyLight(Int3 _pos, uint8_t _val) {
		if (!InBounds(_pos.y))
			return;
		subChunks[size_t(_pos.y >> 4)].SetSkyLight(SubChunk::LocalIndex(_pos), _val);
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

	// Compacts each layer of slice _index (blocks, meta, block light, sky light) on its own, freeing
	// every layer that is uniform. Returns true if at least one layer was freed. Compact() does this
	// for every slice.
	bool TryToCompactSubChunk(int _index);
	void Compact();
	// Deep copies blocks, light and meta
	void CopyStorageFrom(const Chunk& _other);

	// Number of slices with at least one layer allocated
	inline int AllocatedSubChunks() const {
		int n = 0;
		for (const auto& sub : subChunks)
			n += sub.HasAnyLayer() ? 1 : 0;
		return n;
	}
	// Approximate heap + inline footprint. Prefer this over sizeof(Chunk) now that storage is lazy.
	inline size_t GetMemoryUsage() const {
		size_t bytes = sizeof(Chunk);
		for (const auto& sub : subChunks)
			bytes += sub.GetHeapUsage();
		return bytes;
	}
};
