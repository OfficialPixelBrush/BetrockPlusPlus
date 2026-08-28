/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include "./helpers/java/java_math.h"
#include <string_view>
namespace Direction {

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
	None = 6,
};

constexpr Value Opposite(const Value _dir) {
	if (_dir == Value::None)
		return Value::None;
	return static_cast<Value>(static_cast<uint8_t>(_dir) ^ 1);
}

// Rotating around the vertical (Y) axis, as seen from above.
constexpr Value ToLeft(const Value _dir) {
	switch (_dir) {
	case Value::North:
		return Value::West;
	case Value::South:
		return Value::East;
	case Value::East:
		return Value::North;
	case Value::West:
		return Value::South;
	default:
		return _dir;
	}
}

constexpr Value ToRight(const Value _dir) {
	switch (_dir) {
	case Value::North:
		return Value::East;
	case Value::South:
		return Value::West;
	case Value::East:
		return Value::South;
	case Value::West:
		return Value::North;
	default:
		return _dir;
	}
}

constexpr Value FromAngle(const float _yaw) {
	switch (MathHelper::FloorDouble((_yaw + 180.0f) * 4.0f / 360.0f - 0.5f) & 0b11) {
	case 0:
		return Value::East;
	case 1:
		return Value::South;
	case 2:
		return Value::West;
	case 3:
		return Value::North;
	default:
		return Value::None;
	}
}

constexpr bool IsHorizontal(const Value _dir) {
	switch (_dir) {
	case Value::North:
	case Value::South:
	case Value::East:
	case Value::West:
		return true;
	default:
		return false;
	}
}

constexpr bool IsVertical(const Value _dir) {
	switch (_dir) {
	case Value::Up:
	case Value::Down:
		return true;
	default:
		return false;
	}
}

constexpr std::string_view Str(const Value _dir) {
	switch (_dir) {
	case Value::North:
		return "North";
	case Value::South:
		return "South";
	case Value::East:
		return "East";
	case Value::West:
		return "West";
	case Value::Up:
		return "Up";
	case Value::Down:
		return "Down";
	default:
		return "INVALID";
	}
}

} // namespace Direction