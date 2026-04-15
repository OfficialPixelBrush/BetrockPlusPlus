/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include <cmath>
#include <numeric_structs.h>

// This struct stores the client's position as well as some helper functions to get their current chunk and exact block coords. 
// This is used by the world manager to determine which chunks to load and unload around a player
struct ClientPosition {
	Vec3 pos{ 0.0, 0.0, 0.0 };

	inline Int2 getChunkPos() const {
		return Int2{
			static_cast<int>(std::floor(pos.x)) >> 4,
			static_cast<int>(std::floor(pos.z)) >> 4
		};
	}

	inline Int3 getBlockPos() const {
		return Int3{
			static_cast<int>(std::floor(pos.x)),
			static_cast<int>(std::floor(pos.y)),
			static_cast<int>(std::floor(pos.z))
		};
	}

	inline Int2 getRegionPos() const {
		return Int2{
			static_cast<int>(std::floor(pos.x)) >> 9,
			static_cast<int>(std::floor(pos.z)) >> 9
		};
	}
};