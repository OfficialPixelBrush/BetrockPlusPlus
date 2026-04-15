/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
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

enum class ChunkState : uint8_t {
	Unloaded,
	Generating,
	Generated,
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
	uint8_t blocks[4096] = { 0 };
	// Block light and Sky light are both stored in the same array. Lo = Block, Hi = Sky
	uint8_t lightNibble[4096] = { 0 };
	// Block meta is 4 bits each, so each entry is two block's metadata.
	uint8_t nibbleBlockMeta[2048] = { 0 };
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
	inline uint8_t getBlock(Int3 pos) const {
		return blocks[blockIndex(pos)];
	}

	inline void setBlock(Int3 pos, uint8_t id) {
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
	Slice slices[8]; // 128 blocks high
	std::atomic<ChunkState> state{ ChunkState::Unloaded };
	bool isTerrainPopulated = false;
	bool isModified = false; // Whether this chunk has been modified since it was loaded/generated
	uint8_t heightMap[256] = {}; // heightMap[(z << 4) | x]
	float temperature[256] = {};
	float humidity[256] = {};

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
		for (int x = 0; x < 16; x++) {
			for (int z = 0; z < 16; z++) {
				generateHeightMapColumn({ x, z });
			}
		}
	}

	inline void generateHeightMapColumn(Int2 pos) {
		for (uint8_t y = 127; y >= 0; y--) {
			if (Blocks::blockProperties[getBlock({ pos.x, y, pos.z })].lightOpacity > 0) {
				setHeightValue(pos, y + 1);
				return;
			}
		}
		setHeightValue(pos, 0);  // fully transparent column
	}

	inline void generateSkylightMap() {
		generateHeightMap();

		for (int x = 0; x < 16; x++) {
			for (int z = 0; z < 16; z++) {
				int height = getHeightValue({ x, z });

				// Above the heightmap: unconditionally full sky
				for (int y = 127; y >= height; y--)
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
		for (int y = 127; y >= height; y--)
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
		return pos.x >= 0 && pos.x < 16 && pos.y >= 0 && pos.y < 128 && pos.z >= 0 && pos.z < 16;
	}

	// Slice helpers
	inline Slice& getSlice(int y) {
		return slices[y >> 4];
	}
	inline const Slice& getSlice(int y) const {
		return slices[y >> 4];
	}

	// Block helpers
	inline uint8_t getBlock(Int3 pos) const {
		if (!isValidBlockPos(pos)) return 0;
		return getSlice(pos.y).getBlock({pos.x, pos.y & 15, pos.z});
	}

	inline void setBlock(Int3 pos, uint8_t id) {
		if (!isValidBlockPos(pos)) return;
		getSlice(pos.y).setBlock({pos.x, pos.y & 15, pos.z}, id);
		isModified = true;
	}

	// Meta helpers
	inline uint8_t getMeta(Int3 pos) const {
		if (!isValidBlockPos(pos)) return 0;
		return getSlice(pos.y).getMeta({pos.x, pos.y & 15, pos.z});
	}

	inline void setMeta(Int3 pos, uint8_t meta) {
		if (!isValidBlockPos(pos)) return;
		getSlice(pos.y).setMeta({pos.x, pos.y & 15, pos.z}, meta);
		isModified = true;
	}

	// Light helpers
	inline uint8_t getBlockLight(Int3 pos) const {
		if (!isValidBlockPos(pos)) return 0;
		return getSlice(pos.y).getBlockLight({ pos.x, pos.y & 15, pos.z });
	}

	inline uint8_t getSkyLight(Int3 pos) const {
		if (!isValidBlockPos(pos)) return 15;
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
		cpos = {};
		isTerrainPopulated = false;
		isModified = false;
		state.store(ChunkState::Unloaded);
		memset(heightMap, 0, sizeof(heightMap));
		memset(temperature, 0, sizeof(temperature));
		memset(humidity, 0, sizeof(humidity));
		for (auto& s : slices) s.clear();
	}
};