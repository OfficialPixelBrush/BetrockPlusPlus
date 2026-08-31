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
#include <atomic>
#include <cstdint>
#include <cstring>
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

struct Chunk {
	static constexpr int VOLUME = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;
	static constexpr int META_VOLUME = VOLUME / 2;

	Int32_2 cpos;
	std::atomic_bool inUse{ false };

	// Flat arrays indexed by (y * CHUNK_WIDTH * CHUNK_WIDTH) + (z * CHUNK_WIDTH) + x
	BlockType blocks[VOLUME] = { BLOCK_AIR };
	uint8_t lightNibble[VOLUME] = { 0 };
	uint8_t nibbleBlockMeta[META_VOLUME] = { 0 };

	std::atomic<ChunkState> state{ ChunkState::Unloaded };
	uint8_t heightMap[CHUNK_AREA] = {};
	float temperature[CHUNK_AREA] = {};
	float humidity[CHUNK_AREA] = {};
	PackedArray<CHUNK_AREA, 4> biomes;

	bool isTerrainPopulated : 1 = false;
	bool isModified : 1 = false;
	bool spawnChunk : 1 = false;

	// Tile entities
	std::vector<std::shared_ptr<TileEntity>> tileEntities;

	// Used for loading entities into the world from disk
	std::vector<Tag> entityTags;

	inline int BlockIndex(Int3 _pos) const {
		return (_pos.y * CHUNK_WIDTH * CHUNK_WIDTH) + (_pos.z * CHUNK_WIDTH) + _pos.x;
	}

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
		return blocks[BlockIndex(_pos)];
	}
	inline void SetBlock(Int3 _pos, BlockType _id) {
		blocks[BlockIndex(_pos)] = _id;
		isModified = true;
	}
	inline uint8_t GetMeta(Int3 _pos) const {
		int idx = BlockIndex(_pos);
		uint8_t byte = nibbleBlockMeta[idx >> 1];
		return (idx & 1) ? GetNibbleHigh(byte) : GetNibbleLow(byte);
	}
	inline void SetMeta(Int3 _pos, uint8_t _meta) {
		int idx = BlockIndex(_pos);
		uint8_t& byte = nibbleBlockMeta[idx >> 1];
		byte = (idx & 1) ? SetNibble(_meta, GetNibbleLow(byte)) : SetNibble(GetNibbleHigh(byte), _meta);
		isModified = true;
	}
	inline uint8_t GetBlockLight(Int3 _pos) const {
		return GetNibbleLow(lightNibble[BlockIndex(_pos)]);
	}
	inline uint8_t GetSkyLight(Int3 _pos) const {
		return GetNibbleHigh(lightNibble[BlockIndex(_pos)]);
	}
	inline void SetBlockLight(Int3 _pos, uint8_t _val) {
		uint8_t& byte = lightNibble[BlockIndex(_pos)];
		byte = SetNibble(GetNibbleHigh(byte), _val);
		isModified = true;
	}
	inline void SetSkyLight(Int3 _pos, uint8_t _val) {
		uint8_t& byte = lightNibble[BlockIndex(_pos)];
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
};
