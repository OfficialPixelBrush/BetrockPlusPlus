/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once

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
	void FillRegion(Int3 _posA, Int3 _posB, BlockType _type = BLOCK_AIR, uint8_t _meta = 0);

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