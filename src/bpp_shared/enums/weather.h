/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include <cstdint>

// Used as the identifier for the current weather

enum class Weather : int8_t {
    Clear,
    Raining,
    Thundering
};