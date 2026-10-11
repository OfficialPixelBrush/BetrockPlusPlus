/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include "./helpers/java/java_math.h"
#include <string_view>

namespace Axis {
// Axis Directions
enum class Value : uint8_t {
	NorthSouth = 0,
	EastWest = 1,
	UpDown = 2,
	None,
};
inline constexpr Value ALL[3]        = { Value::NorthSouth, Value::EastWest, Value::UpDown };
inline constexpr Value HORIZONTAL[2] = { Value::NorthSouth, Value::EastWest };

constexpr Value ToLeft(const Value _axis);
constexpr Value ToRight(const Value _axis);
constexpr Value FromAngle(const float _yaw);
constexpr bool IsHorizontal(const Value _axis);
constexpr bool IsVertical(const Value _axis);
constexpr std::string_view Str(const Value _axis);
} // namespace Axis

namespace Cardinal {
// Cardinal Directions
enum class Value : uint8_t {
	// Do NOT change this order!
	// This order allows us to get the
	// opposite direction with a single XOR!
	North = 0,
	South = 1,
	East = 2,
	West = 3,
	Up = 4,
	Down = 5,
	None,
};
inline constexpr Value ALL[6]        = { Value::North, Value::South, Value::East, Value::West, Value::Up, Value::Down };
inline constexpr Value HORIZONTAL[4] = { Value::North, Value::South, Value::East, Value::West };

constexpr Value Opposite(const Value _dir);
constexpr Value ToLeft(const Value _dir);
constexpr Value ToRight(const Value _dir);
constexpr Value FromAngle(const float _yaw);
constexpr bool IsHorizontal(const Value _dir);
constexpr bool IsVertical(const Value _dir);
constexpr Axis::Value ToAxis(const Value _dir);
constexpr std::string_view Str(const Value _dir);
} // namespace Cardinal