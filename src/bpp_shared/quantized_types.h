/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once

#include <cstdint>
#include <ostream>
#include <type_traits>
#include "numeric_structs.h"
#include "helpers/byteswap_compat.h"

template <typename Storage = int32_t, int Scale = 32>
struct Fixed {
    static_assert(std::is_integral_v<Storage>,
                  "Storage must be an integer type");

    using storage_type = Storage;

    static constexpr Storage SCALE = Scale;

    Storage m_value;

    constexpr Fixed()
        : m_value(0) {}

    constexpr Fixed(const Fixed&) = default;
    constexpr Fixed& operator=(const Fixed&) = default;

    template <typename T,
              typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    constexpr Fixed(T value)
        : m_value(static_cast<Storage>(value * SCALE)) {}

    static constexpr Fixed from_raw(Storage raw) {
        Fixed result;
        result.m_value = raw;
        return result;
    }

    constexpr Storage raw() const {
        return m_value;
    }

    constexpr double value() const {
        return static_cast<double>(m_value) / SCALE;
    }

    constexpr Fixed operator+(Fixed other) const {
        return from_raw(m_value + other.m_value);
    }

    constexpr Fixed operator-(Fixed other) const {
        return from_raw(m_value - other.m_value);
    }

    constexpr Fixed operator-() const {
        return from_raw(-m_value);
    }

    constexpr Fixed& operator+=(Fixed other) {
        m_value += other.m_value;
        return *this;
    }

    constexpr Fixed& operator-=(Fixed other) {
        m_value -= other.m_value;
        return *this;
    }

    constexpr Fixed operator*(Fixed other) const {
        using Wide = std::conditional_t<
            sizeof(Storage) <= 4,
            int64_t,
            __int128_t
        >;

        return from_raw(
            static_cast<Storage>(
                (static_cast<Wide>(m_value) *
                 static_cast<Wide>(other.m_value)) / SCALE
            )
        );
    }

    constexpr Fixed operator/(Fixed other) const {
        using Wide = std::conditional_t<
            sizeof(Storage) <= 4,
            int64_t,
            __int128_t
        >;

        return from_raw(
            static_cast<Storage>(
                (static_cast<Wide>(m_value) * SCALE) /
                static_cast<Wide>(other.m_value)
            )
        );
    }

    constexpr bool operator==(Fixed other) const {
        return m_value == other.m_value;
    }

    constexpr bool operator!=(Fixed other) const {
        return m_value != other.m_value;
    }

    constexpr bool operator<(Fixed other) const {
        return m_value < other.m_value;
    }

    constexpr bool operator>(Fixed other) const {
        return m_value > other.m_value;
    }

    constexpr bool operator<=(Fixed other) const {
        return m_value <= other.m_value;
    }

    constexpr bool operator>=(Fixed other) const {
        return m_value >= other.m_value;
    }

    friend std::ostream& operator<<(std::ostream& os, Fixed n) {
        return os << n.value();
    }
};

typedef TriNumber<Fixed<int8_t, 32>> NetworkEntityOffset;
typedef TriNumber<Fixed<int32_t, 32>> NetworkEntityPosition;
typedef TriNumber<Fixed<int16_t, 8000>> NetworkEntityVelocity;