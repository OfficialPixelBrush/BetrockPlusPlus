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
        template<typename A, typename B>
        static constexpr auto max(A a, B b) {
            return (a > b) ? a : b;
        }

        template<typename A, typename B>
        static constexpr auto min(A a, B b) {
            return (a < b) ? a : b;
        }
    };
};