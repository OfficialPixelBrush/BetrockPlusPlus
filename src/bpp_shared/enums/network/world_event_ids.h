/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <cstdint>

// Used by the World Event Packet (0x3D)

enum NetworkWorldEvent : int32_t {
	CLICK2			= 1000,
	CLICK1			= 1001,
	BOW_FIRE		= 1002,
	DOOR_TOGGLE		= 1003,
	EXTINGUISH		= 1004,
	RECORD_PLAY		= 1005,
	SMOKE			= 2000,
	BLOCK_BREAK 	= 2001
};