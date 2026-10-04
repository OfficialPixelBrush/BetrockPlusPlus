/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
// Some math helper functions for specific rounding behaviors

namespace Math {
template <typename T = int>
constexpr T CeilDiv(T _x, T _d)
{
    return _x / _d + (_x % _d != 0 && _x > 0);
}

template <typename T = int>
constexpr T FloorDiv(T _x, T _d)
{
    return _x / _d - (_x % _d != 0 && _x < 0);
}
};