#pragma once
#include <cstdint>
#include <hash_map>

struct chunkPos {
	int x = 0;
	int z = 0;

	bool operator==(const chunkPos& other) const {
		return x == other.x && z == other.z;
	}
};

// Hash so we can use chunk pos in unordered maps and sets
template<> 
struct std::hash<chunkPos> {
	std::size_t operator()(chunkPos const& s) const noexcept {
		return ((uint64_t)(uint32_t)s.x << 32) | (uint32_t)s.z;
	}
};

// 16x16x16 blocks of a chunk
struct slice {
	uint8_t blocks[4096] = { 0 };
	// Block light and Sky light are both stored in the same array. Lo = Block, Hi = Sky
	uint8_t lightNibble[4096] = { 0 };
	// Block meta is 4 bits each, so each entry is two block's metadata.
	uint8_t nibbleBlockMeta[2048] = { 0 };

	// So we can skip empty slices when rendering and meshing
	bool isEmpty = true; 
};