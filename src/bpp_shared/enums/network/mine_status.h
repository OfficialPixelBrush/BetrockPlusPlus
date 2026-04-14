/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <cstdint>

enum MineStatus : int8_t {
    DIGGING_STARTED     = 0,
    DIGGING_FINISHED    = 2,
    DROPPED_ITEM        = 4
};