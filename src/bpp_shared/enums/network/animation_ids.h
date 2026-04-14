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
	// The player swings their arm (e.g. when attacking or using an item)
	ANIMATION_PUNCH		= 1,
	// While this still works as intended in b1.7.3,
	// the server does not send it.
	// Instead Entity Event (0x26) is used
	ANIMATION_DAMAGE	= 2,
	// The player is forced to leave the bed
	ANIMATION_LEAVE_BED = 3,
	// An animation that seems to no longer
	// have any connected functionality in b1.7.3
	ANIMATION_REMOVED	= 4
};