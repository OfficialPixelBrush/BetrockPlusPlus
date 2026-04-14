/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <cstdint>

enum FaceDirection : int8_t {
	Y_MINUS	= 0,
	Y_PLUS	= 1,
	Z_MINUS = 2,
	Z_PLUS	= 3,
	X_MINUS = 4,
	X_PLUS	= 5
};