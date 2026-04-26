/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include <cstdint>
#include <functional>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <numeric_structs.h>
#include "blocks/block_properties.h"
#include "constants.h"

enum class ChunkState : uint8_t {
	Unloaded,
	Generating,
	Generated,
	Poulating,
	Populated,
	Lighting,
	Lit,
	Unloading
};

struct ChunkPos {
	int x = 0;
	int z = 0;
	bool operator==(const ChunkPos& other) const {
		return x == other.x && z == other.z;
	}

	uint64_t getHash() const {
		return (uint64_t(x) << 32) | uint64_t(z);
	}
};

// Hash so we can use chunk pos in unordered maps and sets
template<>
struct std::hash<ChunkPos> {
	std::size_t operator()(const ChunkPos& p) const noexcept {
		return std::hash<uint64_t>{}(p.getHash());
	}
};

// 16x16x16 blocks of a chunk
struct Slice {
	BlockType blocks[SUB_CHUNK_VOLUME] = { BLOCK_AIR };
	// Block light and Sky light are both stored in the same array. Lo = Block, Hi = Sky
	uint8_t lightNibble[SUB_CHUNK_VOLUME] = { 0 };
	// Block meta is 4 bits each, so each entry is two block's metadata.
	uint8_t nibbleBlockMeta[SUB_CHUNK_VOLUME / 2] = { 0 };
	// So we can skip empty slices when rendering and meshing
	bool isEmpty = true;

	// Calculate the index in the block array for a given block position within this slice
	inline int blockIndex(Int3 pos) const {
		return (pos.y << 8) | (pos.z << 4) | pos.x;
	}

	// Nibble helpers
	inline uint8_t setNibble(uint8_t hi, uint8_t lo) const {
		return uint8_t(((hi & uint8_t(0x0F)) << 4) | (lo & uint8_t(0x0F)));
	}
	inline uint8_t getNibbleLow(uint8_t byte)  const { return  byte & uint8_t(0x0F); }
	inline uint8_t getNibbleHigh(uint8_t byte) const { return (byte >> 4) & uint8_t(0x0F); }

	// Block helpers
	inline BlockType getBlock(Int3 pos) const {
		return blocks[blockIndex(pos)];
	}

	inline void setBlock(Int3 pos, BlockType id) {
		blocks[blockIndex(pos)] = id;
		if (id != 0) isEmpty = false;
	}

	// Meta helpers
	inline uint8_t getMeta(Int3 pos) const {
		int idx = blockIndex(pos);
		uint8_t byte = nibbleBlockMeta[idx >> 1];
		return (idx & 1) ? getNibbleHigh(byte) : getNibbleLow(byte);
	}

	inline void setMeta(Int3 pos, uint8_t meta) {
		int idx = blockIndex(pos);
		uint8_t& byte = nibbleBlockMeta[idx >> 1];
		byte = (idx & 1)
			? setNibble(meta, getNibbleLow(byte))
			: setNibble(getNibbleHigh(byte), meta);
	}

	// Light helpers
	inline uint8_t getBlockLight(Int3 pos) const {
		return getNibbleLow(lightNibble[blockIndex(pos)]);
	}

	inline uint8_t getSkyLight(Int3 pos) const {
		return getNibbleHigh(lightNibble[blockIndex(pos)]);
	}

	inline void setBlockLight(Int3 pos, uint8_t val) {
		uint8_t& byte = lightNibble[blockIndex(pos)];
		byte = setNibble(getNibbleHigh(byte), val);
	}

	inline void setSkyLight(Int3 pos, uint8_t val) {
		uint8_t& byte = lightNibble[blockIndex(pos)];
		byte = setNibble(val, getNibbleLow(byte));
	}

	inline int getBlockLightValue(Int3 pos, int skySubtracted) const {
		int sky = std::max(0, int(getSkyLight(pos) - skySubtracted));
		int block = int(getBlockLight(pos));
		return std::min(15, std::max(sky, block));
	}

	// Cleanup
	inline void clear() {
		isEmpty = true;
		std::memset(blocks, 0, sizeof(blocks));
		std::memset(lightNibble, 0, sizeof(lightNibble));
		std::memset(nibbleBlockMeta, 0, sizeof(nibbleBlockMeta));
	}
};

struct Chunk {
	ChunkPos cpos;
	Slice slices[CHUNK_HEIGHT / SUB_CHUNK_SIZE]; // 128 blocks high
	std::atomic<ChunkState> state{ ChunkState::Unloaded };
	uint8_t heightMap[CHUNK_WIDTH * CHUNK_WIDTH] = {}; // heightMap[(z << 4) | x]
	float temperature[CHUNK_WIDTH * CHUNK_WIDTH] = {};
	float humidity[CHUNK_WIDTH * CHUNK_WIDTH] = {};

	bool isTerrainPopulated = false;
	bool isModified = false;

	// Climate helpers
	inline float getTemperature(Int2 pos) const { return temperature[(pos.y << 4) | pos.x]; }
	inline float getHumidity(Int2 pos)    const { return humidity[(pos.y << 4) | pos.x]; }

	// Height map helpers
	inline uint8_t getHeightValue(Int2 pos) const {
		return heightMap[(pos.y << 4) | pos.x];
	}
	inline void setHeightValue(Int2 pos, uint8_t val) {
		heightMap[(pos.y << 4) | pos.x] = val;
	}
	inline bool canBlockSeeSky(Int3 pos) const {
		return pos.y >= getHeightValue({ pos.x, pos.z });
	}

	// Used for population and lighting calculations
	inline void generateHeightMap() {
		for (int x = 0; x < CHUNK_WIDTH; x++) {
			for (int z = 0; z < CHUNK_WIDTH; z++) {
				generateHeightMapColumn({ x, z });
			}
		}
	}

	inline void generateHeightMapColumn(Int2 pos) {
		for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
			if (Blocks::blockProperties[getBlock({ pos.x, y, pos.z })].lightOpacity > 0) {
				setHeightValue(pos, uint8_t(y + 1));
				return;
			}
		}
		setHeightValue(pos, 0);  // fully transparent column
	}

	inline void generateSkylightMap() {
		generateHeightMap();

		for (int x = 0; x < CHUNK_WIDTH; x++) {
			for (int z = 0; z < CHUNK_WIDTH; z++) {
				int height = getHeightValue({ x, z });

				// Above the heightmap: unconditionally full sky
				for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
					setSkyLight({ x, y, z }, 15);

				// Below the heightmap: bleed light through semi-transparent blocks
				int skyLight = 15;
				for (int y = height - 1; y >= 0 && skyLight > 0; y--) {
					skyLight -= std::max(1, int(Blocks::blockProperties[getBlock({ x, y, z })].lightOpacity));
					if (skyLight > 0)
						setSkyLight({ x, y, z }, uint8_t(skyLight));
				}
			}
		}
	}

	inline void relightColumn(Int2 pos) {
		// Rebuild the heightmap for this column
		generateHeightMapColumn(pos);

		int height = getHeightValue(pos);

		// Set full skylight for everything at or above the surface
		for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
			setSkyLight({ pos.x, y, pos.z }, 15);

		// Attenuate below the surface
		int skyLight = 15;
		for (int y = height - 1; y >= 0 && skyLight > 0; y--) {
			skyLight -= std::max(1, int(Blocks::blockProperties[getBlock({ pos.x, y, pos.z })].lightOpacity));
			if (skyLight > 0)
				setSkyLight({ pos.x, y, pos.z }, uint8_t(skyLight));
		}
	}

	// Make sure we can't access a block out of bounds
	inline bool isValidBlockPos(Int3 pos) const {
		bool valid = pos.x >= 0 && pos.x < CHUNK_WIDTH && pos.y >= 0 && pos.y < CHUNK_HEIGHT && pos.z >= 0 && pos.z < CHUNK_WIDTH;
		if (!valid) printf("Invalid block access on chunk x: %i y: %i at block coords x: %i y: %i z: %i\n", cpos.x, cpos.z, pos.x, pos.y, pos.z);
		return valid;
	}

	// Slice helpers
	inline Slice& getSlice(int y) {
		return slices[y >> 4];
	}
	inline const Slice& getSlice(int y) const {
		return slices[y >> 4];
	}

	// Block helpers
	inline BlockType getBlock(Int3 pos) const {
		if (!isValidBlockPos(pos)) return BLOCK_AIR;
		return getSlice(pos.y).getBlock({ pos.x, pos.y & 15, pos.z });
	}

	inline void setBlock(Int3 pos, BlockType id) {
		if (!isValidBlockPos(pos)) return;
		getSlice(pos.y).setBlock({ pos.x, pos.y & 15, pos.z }, id);
		isModified = true;
	}

	// Meta helpers
	inline uint8_t getMeta(Int3 pos) const {
		if (!isValidBlockPos(pos)) return 0;
		return getSlice(pos.y).getMeta({ pos.x, pos.y & 15, pos.z });
	}

	inline void setMeta(Int3 pos, uint8_t meta) {
		if (!isValidBlockPos(pos)) return;
		getSlice(pos.y).setMeta({ pos.x, pos.y & 15, pos.z }, meta);
		isModified = true;
	}

	// Light helpers
	inline uint8_t getBlockLight(Int3 pos) const {
		if (!isValidBlockPos(pos)) return 0;
		return getSlice(pos.y).getBlockLight({ pos.x, pos.y & 15, pos.z });
	}

	inline uint8_t getSkyLight(Int3 pos) const {
		if (!isValidBlockPos(pos)) return 0;
		return getSlice(pos.y).getSkyLight({ pos.x, pos.y & 15, pos.z });
	}

	inline void setBlockLight(Int3 pos, uint8_t val) {
		if (!isValidBlockPos(pos)) return;
		getSlice(pos.y).setBlockLight({ pos.x, pos.y & 15, pos.z }, val);
		isModified = true;
	}

	inline void setSkyLight(Int3 pos, uint8_t val) {
		if (!isValidBlockPos(pos)) return;
		getSlice(pos.y).setSkyLight({ pos.x, pos.y & 15, pos.z }, val);
		isModified = true;
	}

	inline int getBlockLightValue(Int3 pos, int skySubtracted) const {
		if (!isValidBlockPos(pos)) return 15 - skySubtracted;
		return getSlice(pos.y).getBlockLightValue({ pos.x, pos.y & 15, pos.z }, skySubtracted);
	}

	// cleanup
	inline void clear() {
		isTerrainPopulated = false;
		isModified = false;
		memset(heightMap, 0, sizeof(heightMap));
		memset(temperature, 0, sizeof(temperature));
		memset(humidity, 0, sizeof(humidity));
		for (auto& s : slices) s.clear();
	}
};