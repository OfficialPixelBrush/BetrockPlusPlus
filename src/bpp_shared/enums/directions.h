/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once

/*
 * TODO:	Is this actually useful for anything?
 *			I think this is only used for determining which
 *			face of a block was clicked on the server when
 *			placing or destroying blocks
*/
enum FaceDirection {
	yMinus = 0,
	yPlus = 1,
	zMinus = 2,
	zPlus = 3,
	xMinus = 4,
	xPlus = 5
};