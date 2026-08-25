/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include "./helpers/java/java_math.h"
namespace Direction {

enum class Value : uint8_t {
    None,
    North,
    South,
    East,
    West,
    Up,
    Down
};

constexpr Value Opposite(const Value _dir) {
    switch (_dir) {
    case Value::North: return Value::South;
    case Value::South: return Value::North;
    case Value::East:  return Value::West;
    case Value::West:  return Value::East;
    case Value::Up:    return Value::Down;
    case Value::Down:  return Value::Up;
    default:              return _dir;
    }
}

// Rotating around the vertical (Y) axis, as seen from above.
constexpr Value ToLeft(const Value _dir) {
    switch (_dir) {
    case Value::North: return Value::West;
    case Value::South: return Value::East;
    case Value::East:  return Value::North;
    case Value::West:  return Value::South;
    default:              return _dir;
    }
}

constexpr Value ToRight(const Value _dir) {
    switch (_dir) {
    case Value::North: return Value::East;
    case Value::South: return Value::West;
    case Value::East:  return Value::South;
    case Value::West:  return Value::North;
    default:              return _dir;
    }
}

constexpr Value FromAngle(const float _yaw) {
    switch(MathHelper::FloorDouble((_yaw + 180.0f) * 4.0f / 360.0f - 0.5f) & 0b11) {
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

}