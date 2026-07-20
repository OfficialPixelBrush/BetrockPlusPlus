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
#include "helpers/cross_platform.h"
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
	static constexpr int M_VOLUME = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;
	static constexpr int M_META_VOLUME = M_VOLUME / 2;

	Int32_2 cpos;
	std::atomic_bool inUse{ false };

	// Flat arrays indexed by (y * CHUNK_WIDTH * CHUNK_WIDTH) + (z * CHUNK_WIDTH) + x
	BlockType blocks[M_VOLUME] = { BLOCK_AIR };
	uint8_t lightNibble[M_VOLUME] = { 0 };
	uint8_t nibbleBlockMeta[M_META_VOLUME] = { 0 };

	std::atomic<ChunkState> state{ ChunkState::Unloaded };
	uint8_t heightMap[CHUNK_WIDTH * CHUNK_WIDTH] = {};
	float temperature[CHUNK_WIDTH * CHUNK_WIDTH] = {};
	float humidity[CHUNK_WIDTH * CHUNK_WIDTH] = {};

	bool isTerrainPopulated = false;
	bool isModified = false;
	bool spawnChunk = false;

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
	inline bool CanBlockSeeSky(Int3 _pos) const {
		return _pos.y >= GetHeightValue({ _pos.x, _pos.z });
	}

	inline void GenerateHeightMap() {
		for (int x = 0; x < CHUNK_WIDTH; x++)
			for (int z = 0; z < CHUNK_WIDTH; z++)
				GenerateHeightMapColumn({ x, z });
	}

	inline void GenerateHeightMapColumn(Int2 _pos) {
		for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
			if (Blocks::blockProperties[GetBlock({ _pos.x, y, _pos.z })].lightOpacity > 0) {
				SetHeightValue(_pos, uint8_t(y + 1));
				return;
			}
		}
		SetHeightValue(_pos, 0);
	}

	inline void GenerateSkylightMap() {
		GenerateHeightMap();
		for (int x = 0; x < CHUNK_WIDTH; x++) {
			for (int z = 0; z < CHUNK_WIDTH; z++) {
				int height = GetHeightValue({ x, z });
				for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
					SetSkyLight({ x, y, z }, 15);
				int skyLight = 15;
				for (int y = height - 1; y >= 0; y--) {
					skyLight -= CrossPlatform::Math::Max(
					    1, int(Blocks::blockProperties[GetBlock({ x, y, z })].lightOpacity));
					skyLight = CrossPlatform::Math::Max(0, skyLight);
					SetSkyLight({ x, y, z }, uint8_t(skyLight));
				}
			}
		}
	}

	inline void RelightColumn(Int2 _pos) {
		GenerateHeightMapColumn(_pos);
		int height = GetHeightValue(_pos);

		for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
			SetSkyLight({ _pos.x, y, _pos.z }, 15);

		// only pull values up
		int skyLight = 15;
		for (int y = height - 1; y >= 0; y--) {
			skyLight -= CrossPlatform::Math::Max(
			    1, int(Blocks::blockProperties[GetBlock({ _pos.x, y, _pos.z })].lightOpacity));
			skyLight = CrossPlatform::Math::Max(0, skyLight);
			uint8_t current = GetSkyLight({ _pos.x, y, _pos.z });
			if (current < skyLight)
				SetSkyLight({ _pos.x, y, _pos.z }, uint8_t(skyLight));
		}
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

	inline void Clear() {
		isTerrainPopulated = false;
		isModified = false;
		std::memset(blocks, 0, sizeof(blocks));
		std::memset(lightNibble, 0, sizeof(lightNibble));
		std::memset(nibbleBlockMeta, 0, sizeof(nibbleBlockMeta));
		std::memset(heightMap, 0, sizeof(heightMap));
		std::memset(temperature, 0, sizeof(temperature));
		std::memset(humidity, 0, sizeof(humidity));
	}
};