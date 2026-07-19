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
	static constexpr int VOLUME = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;
	static constexpr int META_VOLUME = VOLUME / 2;

	Int32_2 m_cpos;
	std::atomic_bool m_inUse{ false };

	// Flat arrays indexed by (y * CHUNK_WIDTH * CHUNK_WIDTH) + (z * CHUNK_WIDTH) + x
	BlockType m_blocks[VOLUME] = { BLOCK_AIR };
	uint8_t m_lightNibble[VOLUME] = { 0 };
	uint8_t m_nibbleBlockMeta[META_VOLUME] = { 0 };

	std::atomic<ChunkState> m_state{ ChunkState::Unloaded };
	uint8_t m_heightMap[CHUNK_WIDTH * CHUNK_WIDTH] = {};
	float m_temperature[CHUNK_WIDTH * CHUNK_WIDTH] = {};
	float m_humidity[CHUNK_WIDTH * CHUNK_WIDTH] = {};

	bool m_isTerrainPopulated = false;
	bool m_isModified = false;
	bool m_spawnChunk = false;

	// Tile entities
	std::vector<std::shared_ptr<TileEntity>> m_tileEntities;

	// Used for loading entities into the world from disk
	std::vector<Tag> m_entityTags;

	inline int blockIndex(Int3 pos) const {
		return (pos.m_y * CHUNK_WIDTH * CHUNK_WIDTH) + (pos.m_z * CHUNK_WIDTH) + pos.m_x;
	}

	inline uint8_t setNibble(uint8_t hi, uint8_t lo) const {
		return uint8_t(((hi & 0x0Fu) << 4) | (lo & 0x0Fu));
	}
	inline uint8_t getNibbleLow(uint8_t byte) const {
		return byte & 0x0Fu;
	}
	inline uint8_t getNibbleHigh(uint8_t byte) const {
		return (byte >> 4) & 0x0Fu;
	}

	inline float getTemperature(Int2 pos) const {
		return m_temperature[(pos.m_x << 4) | pos.m_y];
	}
	inline float getHumidity(Int2 pos) const {
		return m_humidity[(pos.m_x << 4) | pos.m_y];
	}

	inline uint8_t getHeightValue(Int2 pos) const {
		return m_heightMap[(pos.m_y << 4) | pos.m_x];
	}
	inline void setHeightValue(Int2 pos, uint8_t val) {
		m_heightMap[(pos.m_y << 4) | pos.m_x] = val;
	}
	inline bool canBlockSeeSky(Int3 pos) const {
		return pos.m_y >= getHeightValue({ pos.m_x, pos.m_z });
	}

	inline void generateHeightMap() {
		for (int x = 0; x < CHUNK_WIDTH; x++)
			for (int z = 0; z < CHUNK_WIDTH; z++)
				generateHeightMapColumn({ x, z });
	}

	inline void generateHeightMapColumn(Int2 pos) {
		for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
			if (Blocks::blockProperties[getBlock({ pos.m_x, y, pos.m_z })].m_lightOpacity > 0) {
				setHeightValue(pos, uint8_t(y + 1));
				return;
			}
		}
		setHeightValue(pos, 0);
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
					    1, int(Blocks::blockProperties[getBlock({ x, y, z })].m_lightOpacity));
					skyLight = CrossPlatform::Math::max(0, skyLight);
					setSkyLight({ x, y, z }, uint8_t(skyLight));
				}
			}
		}
	}

	inline void relightColumn(Int2 pos) {
		generateHeightMapColumn(pos);
		int height = getHeightValue(pos);

		for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
			setSkyLight({ pos.m_x, y, pos.m_z }, 15);

		// only pull values up
		int skyLight = 15;
		for (int y = height - 1; y >= 0; y--) {
			skyLight -= CrossPlatform::Math::max(
			    1, int(Blocks::blockProperties[getBlock({ pos.m_x, y, pos.m_z })].m_lightOpacity));
			skyLight = CrossPlatform::Math::max(0, skyLight);
			uint8_t current = getSkyLight({ pos.m_x, y, pos.m_z });
			if (current < skyLight)
				setSkyLight({ pos.m_x, y, pos.m_z }, uint8_t(skyLight));
		}
	}

	inline BlockType getBlock(Int3 pos) const {
		return m_blocks[blockIndex(pos)];
	}

	inline void setBlock(Int3 pos, BlockType id) {
		m_blocks[blockIndex(pos)] = id;
		m_isModified = true;
	}

	inline uint8_t getMeta(Int3 pos) const {
		int idx = blockIndex(pos);
		uint8_t byte = m_nibbleBlockMeta[idx >> 1];
		return (idx & 1) ? getNibbleHigh(byte) : getNibbleLow(byte);
	}

	inline void setMeta(Int3 pos, uint8_t meta) {
		int idx = blockIndex(pos);
		uint8_t& byte = m_nibbleBlockMeta[idx >> 1];
		byte = (idx & 1) ? setNibble(meta, getNibbleLow(byte)) : setNibble(getNibbleHigh(byte), meta);
		m_isModified = true;
	}

	inline uint8_t getBlockLight(Int3 pos) const {
		return getNibbleLow(m_lightNibble[blockIndex(pos)]);
	}

	inline uint8_t getSkyLight(Int3 pos) const {
		return getNibbleHigh(m_lightNibble[blockIndex(pos)]);
	}

	inline void setBlockLight(Int3 pos, uint8_t val) {
		uint8_t& byte = m_lightNibble[blockIndex(pos)];
		byte = setNibble(getNibbleHigh(byte), val);
		m_isModified = true;
	}

	inline void setSkyLight(Int3 pos, uint8_t val) {
		uint8_t& byte = m_lightNibble[blockIndex(pos)];
		byte = setNibble(val, getNibbleLow(byte));
		m_isModified = true;
	}

	inline int getBlockLightValue(Int3 pos, int skySubtracted) const {
		int sky = CrossPlatform::Math::max(0, int(getSkyLight(pos)) - skySubtracted);
		int block = int(getBlockLight(pos));
		return CrossPlatform::Math::min(15, CrossPlatform::Math::max(sky, block));
	}

	inline void clear() {
		m_isTerrainPopulated = false;
		m_isModified = false;
		std::memset(m_blocks, 0, sizeof(m_blocks));
		std::memset(m_lightNibble, 0, sizeof(m_lightNibble));
		std::memset(m_nibbleBlockMeta, 0, sizeof(m_nibbleBlockMeta));
		std::memset(m_heightMap, 0, sizeof(m_heightMap));
		std::memset(m_temperature, 0, sizeof(m_temperature));
		std::memset(m_humidity, 0, sizeof(m_humidity));
	}
};