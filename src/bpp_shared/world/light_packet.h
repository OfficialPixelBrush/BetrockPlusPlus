/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include <cstdint>
#include <numeric_structs.h>

// Type of light
enum LightType {
	SkyLight,
	BlockLight
};

// Used by the global lighter
struct LightPacket {
	LightType light_type;
	uint8_t light_level = 0;
	Int3 local_position{ 0, 0, 0 };
};