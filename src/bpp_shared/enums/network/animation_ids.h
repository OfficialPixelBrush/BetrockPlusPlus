/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <cstdint>

// Used by the Animation Packet (0x12)

enum NetworkAnimation : int32_t {
	ANIMATION_NONE		= 0,
	ANIMATION_PUNCH		= 1,
	ANIMATION_DAMAGE	= 2,
	ANIMATION_LEAVE_BED = 3,
	// An animation that seems to no longer
	// have any connected functionality in b1.7.3
	ANIMATION_REMOVED	= 4
};