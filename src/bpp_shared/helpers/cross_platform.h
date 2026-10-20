/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include <type_traits>

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

		// Mixed-type overloads (e.g. int32_t vs int, which are distinct types on
		// some targets such as the PS2's newlib where int32_t is long).
		template <typename A, typename B, typename = std::enable_if_t<!std::is_same_v<A, B>>>
		static inline std::common_type_t<A, B> Min(A _a, B _b) {
			return _a < _b ? _a : _b;
		}

		template <typename A, typename B, typename = std::enable_if_t<!std::is_same_v<A, B>>>
		static inline std::common_type_t<A, B> Max(A _a, B _b) {
			return _a > _b ? _a : _b;
		}
	};
};