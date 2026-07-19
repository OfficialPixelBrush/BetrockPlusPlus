/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include "blocks.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

// Block Struct
struct Block {
	BlockType m_type = BLOCK_AIR;
	uint8_t m_data = 0;

	friend std::ostream& operator<<(std::ostream& os, const Block& b) {
		os << "(" << int32_t(b.m_type) << ":" << int32_t(b.m_data) << ")";
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
	uint8_t m_blocklight = 0;
	uint8_t m_skylight = 0;
};
