#pragma once
#include <array>
#include <cstdint>
#include <numeric_structs.h>

// This is a struct that contains a bunch of relevant data a block may need during meshing; especially liquids
// Makes face lookups a LOT faster than having to go through the world object every time
// The relevant caches are built in the sub chunk mesher

struct BlockData {
	Int3 pos{ 0, 0, 0 };
	uint8_t blockId = 0;
	uint8_t blockMeta = 0;
	float blockLight = 0.0f; // Converted value from 0-15 to the range 0.0-1.0 using the specified dimension's brightness curve
};

// This struct is a little heavy but since each mesher only has one instance of this render context at a time it should be fine
struct BlockRenderContext {
	float temperature = 0.0f;
	float humidity = 0.0f;
	int worldX = 0;
	int worldY = 0;
	int worldZ = 0;
	bool fancyGraphics = false;
	bool smoothLighting = false;

	// 3x3x3 cube of blocks centered on the block being rendered
	// You can access the current block by requestion the neighbor with dx=0, dy=0, dz=0
	std::array<BlockData, 27> neighbors; 

	int neighborIndex(int dx, int dy, int dz) {
		return (dy + 1) * 9 + (dz + 1) * 3 + (dx + 1);
	}

	BlockData& getNeighbor(int dx, int dy, int dz) {
		return neighbors[neighborIndex(dx, dy, dz)];
	}
};