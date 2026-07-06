/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include "blocks.h"

// Block Struct
struct Block {
	BlockType type = BLOCK_AIR;
	uint8_t data = 0;

	friend std::ostream& operator<<(std::ostream& os, const Block& b) {
		os << "(" << int32_t(b.type) << ":" << int32_t(b.data) << ")";
		return os;
	}

	std::string str() const {
		std::ostringstream oss;
		oss << *this; // Use the overloaded << operator
		return oss.str();
	}
};

// Lighting + Block Struct
struct LitBlock : Block {
	uint8_t blocklight = 0;
	uint8_t skylight = 0;
};