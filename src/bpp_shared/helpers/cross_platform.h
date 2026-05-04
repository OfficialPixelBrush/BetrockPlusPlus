/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#define NOMINMAX

struct CrossPlatform {
    struct Math {
        template<typename T = int>
        static inline T min(T a, T b) {
            return a < b ? a : b;
        }

        template<typename T = int>
        static inline T max(T a, T b) {
            return a > b ? a : b;
        }
    };
};