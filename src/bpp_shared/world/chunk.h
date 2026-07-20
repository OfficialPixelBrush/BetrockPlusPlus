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
	static constexpr int m_VOLUME = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;
	static constexpr int m_META_VOLUME = m_VOLUME / 2;

	Int32_2 cpos;
	std::atomic_bool inUse{ false };

	// Flat arrays indexed by (y * CHUNK_WIDTH * CHUNK_WIDTH) + (z * CHUNK_WIDTH) + x
	BlockType blocks[m_VOLUME] = { BLOCK_AIR };
	uint8_t lightNibble[m_VOLUME] = { 0 };
	uint8_t nibbleBlockMeta[m_META_VOLUME] = { 0 };

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

	inline int blockIndex(Int3 _pos) const {
		return (_pos.y * CHUNK_WIDTH * CHUNK_WIDTH) + (_pos.z * CHUNK_WIDTH) + _pos.x;
	}

	inline uint8_t setNibble(uint8_t _hi, uint8_t _lo) const {
		return uint8_t(((_hi & 0x0Fu) << 4) | (_lo & 0x0Fu));
	}
	inline uint8_t getNibbleLow(uint8_t _byte) const {
		return _byte & 0x0Fu;
	}
	inline uint8_t getNibbleHigh(uint8_t _byte) const {
		return (_byte >> 4) & 0x0Fu;
	}

	inline float getTemperature(Int2 _pos) const {
		return temperature[(_pos.x << 4) | _pos.y];
	}
	inline float getHumidity(Int2 _pos) const {
		return humidity[(_pos.x << 4) | _pos.y];
	}

	inline uint8_t getHeightValue(Int2 _pos) const {
		return heightMap[(_pos.y << 4) | _pos.x];
	}
	inline void setHeightValue(Int2 _pos, uint8_t _val) {
		heightMap[(_pos.y << 4) | _pos.x] = _val;
	}
	inline bool canBlockSeeSky(Int3 _pos) const {
		return _pos.y >= getHeightValue({ _pos.x, _pos.z });
	}

	inline void generateHeightMap() {
		for (int x = 0; x < CHUNK_WIDTH; x++)
			for (int z = 0; z < CHUNK_WIDTH; z++)
				generateHeightMapColumn({ x, z });
	}

	inline void generateHeightMapColumn(Int2 _pos) {
		for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
			if (Blocks::blockProperties[getBlock({ _pos.x, y, _pos.z })].lightOpacity > 0) {
				setHeightValue(_pos, uint8_t(y + 1));
				return;
			}
		}
		setHeightValue(_pos, 0);
	}

	inline void generateSkylightMap() {
		generateHeightMap();
		for (int x = 0; x < CHUNK_WIDTH; x++) {
			for (int z = 0; z < CHUNK_WIDTH; z++) {
				int height = getHeightValue({ x, z });
				for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
					setSkyLight({ x, y, z }, 15);
				int skyLight = 15;
				for (int y = height - 1; y >= 0; y--) {
					skyLight -= CrossPlatform::Math::max(
					    1, int(Blocks::blockProperties[getBlock({ x, y, z })].lightOpacity));
					skyLight = CrossPlatform::Math::max(0, skyLight);
					setSkyLight({ x, y, z }, uint8_t(skyLight));
				}
			}
		}
	}

	inline void relightColumn(Int2 _pos) {
		generateHeightMapColumn(_pos);
		int height = getHeightValue(_pos);

		for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
			setSkyLight({ _pos.x, y, _pos.z }, 15);

		// only pull values up
		int skyLight = 15;
		for (int y = height - 1; y >= 0; y--) {
			skyLight -= CrossPlatform::Math::max(
			    1, int(Blocks::blockProperties[getBlock({ _pos.x, y, _pos.z })].lightOpacity));
			skyLight = CrossPlatform::Math::max(0, skyLight);
			uint8_t current = getSkyLight({ _pos.x, y, _pos.z });
			if (current < skyLight)
				setSkyLight({ _pos.x, y, _pos.z }, uint8_t(skyLight));
		}
	}

	inline BlockType getBlock(Int3 _pos) const {
		return blocks[blockIndex(_pos)];
	}

	inline void setBlock(Int3 _pos, BlockType _id) {
		blocks[blockIndex(_pos)] = _id;
		isModified = true;
	}

	inline uint8_t getMeta(Int3 _pos) const {
		int idx = blockIndex(_pos);
		uint8_t byte = nibbleBlockMeta[idx >> 1];
		return (idx & 1) ? getNibbleHigh(byte) : getNibbleLow(byte);
	}

	inline void setMeta(Int3 _pos, uint8_t _meta) {
		int idx = blockIndex(_pos);
		uint8_t& byte = nibbleBlockMeta[idx >> 1];
		byte = (idx & 1) ? setNibble(_meta, getNibbleLow(byte)) : setNibble(getNibbleHigh(byte), _meta);
		isModified = true;
	}

	inline uint8_t getBlockLight(Int3 _pos) const {
		return getNibbleLow(lightNibble[blockIndex(_pos)]);
	}

	inline uint8_t getSkyLight(Int3 _pos) const {
		return getNibbleHigh(lightNibble[blockIndex(_pos)]);
	}

	inline void setBlockLight(Int3 _pos, uint8_t _val) {
		uint8_t& byte = lightNibble[blockIndex(_pos)];
		byte = setNibble(getNibbleHigh(byte), _val);
		isModified = true;
	}

	inline void setSkyLight(Int3 _pos, uint8_t _val) {
		uint8_t& byte = lightNibble[blockIndex(_pos)];
		byte = setNibble(_val, getNibbleLow(byte));
		isModified = true;
	}

	inline int getBlockLightValue(Int3 _pos, int _skySubtracted) const {
		int sky = CrossPlatform::Math::max(0, int(getSkyLight(_pos)) - _skySubtracted);
		int block = int(getBlockLight(_pos));
		return CrossPlatform::Math::min(15, CrossPlatform::Math::max(sky, block));
	}

	inline void clear() {
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