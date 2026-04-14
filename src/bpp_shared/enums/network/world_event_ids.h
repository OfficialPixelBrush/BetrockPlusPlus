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
	// Button click sound
	CLICK2			= 1000,
	// Alt. button click sound
	CLICK1			= 1001,
	// Bow shooting sound
	BOW_FIRE		= 1002,
	// Door opening/closing sound
	DOOR_TOGGLE		= 1003,
	// Extinguish fire sound
	EXTINGUISH		= 1004,
	// Record playing sound, requires music disc item id as parameter
	RECORD_PLAY		= 1005,
	// Smoke particle effect, requires index for a position
	SMOKE			= 2000,
	// Block breaking particle effect, requires block id
	BLOCK_BREAK 	= 2001
};