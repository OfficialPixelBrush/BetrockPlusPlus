/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once

struct CrossPlatform {
	struct Math {
		template <typename T = int>
		static inline T Min(T _a, T _b) {
			return _a < _b ? _a : _b;
		}

		template <typename T = int>
		static inline T Max(T _a, T _b) {
			return _a > _b ? _a : _b;
		}
	};
};