/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <ostream>
#include <sstream>

/**
 * @brief A struct that contains three numbers (x,y,z)
 * 
 */
template<typename T = int>
struct TriNumber {
    union {
        // For accessing directly
        struct {
            T x,y,z;
        };
        // For accessing as array
        T data[3];
    };
    bool operator==(const TriNumber& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    TriNumber operator+(const TriNumber& other) const {
        return TriNumber{x + other.x, y + other.y, z + other.z};
    }

    TriNumber operator-(const TriNumber& other) const {
        return TriNumber{x - other.x, y - other.y, z - other.z};
    }

    TriNumber operator*(const TriNumber& other) const {
        return TriNumber{x * other.x, y * other.y, z * other.z};
    }

    TriNumber operator/(const TriNumber& other) const {
        return TriNumber{x / other.x, y / other.y, z / other.z};
    }

    // Allows for tri + 1 = (x+1,y+1,z+1)
    template<typename U>
    auto operator-(const U& other) const {
        using R = std::common_type_t<T, U>;
        return TriNumber<R>{
            static_cast<R>(x) - other,
            static_cast<R>(y) - other,
            static_cast<R>(z) - other
        };
    }

    // Allows for tri - 1 = (x-1,y-1,z-1)
    template<typename U>
    auto operator+(const U& other) const {
        using R = std::common_type_t<T, U>;
        return TriNumber<R>{
            static_cast<R>(x) + other,
            static_cast<R>(y) + other,
            static_cast<R>(z) + other
        };
    }

    // Allows for tri * 2 = (x*2,y*2,z*2)
    template<typename U>
    auto operator*(const U& other) const {
        using R = std::common_type_t<T, U>;
        return TriNumber<R>{
            static_cast<R>(x) * other,
            static_cast<R>(y) * other,
            static_cast<R>(z) * other
        };
    }

    // Allows for tri / 2 = (x/2,y/2,z/2)
    template<typename U>
    auto operator/(const U& other) const {
        using R = std::common_type_t<T, U>;
        return TriNumber<R>{
            static_cast<R>(x) / other,
            static_cast<R>(y) / other,
            static_cast<R>(z) / other
        };
    }

    friend std::ostream& operator<<(std::ostream& os, const TriNumber& vec) {
        os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
        return os;
    }
    
    std::string str() const {
        std::ostringstream oss;
        oss << *this; // Use the overloaded << operator
        return oss.str();
    }
};

/**
 * @brief A struct that contains two numbers (x,y)
 * 
 */
template<typename T = int>
struct BiNumber {
    union {
        // For accessing directly
        struct {
            T x;
            union {
                T y;
                T z;
            };
        };
        // For accessing as array
        T data[2];
    };
    bool operator==(const BiNumber& other) const {
        return x == other.x && y == other.y;
    }

    BiNumber operator+(const BiNumber& other) const {
        return BiNumber{x + other.x, y + other.y};
    }

    BiNumber operator-(const BiNumber& other) const {
        return BiNumber{x - other.x, y - other.y};
    }

    BiNumber operator*(const BiNumber& other) const {
        return BiNumber{x * other.x, y * other.y};
    }

    BiNumber operator/(const BiNumber& other) const {
        return BiNumber{x / other.x, y / other.y};
    }

    // Allows for bi * 2 = (x*2,y*2)
    template<typename U>
    auto operator*(const U& other) const {
        using R = std::common_type_t<T, U>;
        return TriNumber<R>{
            static_cast<R>(x) * other,
            static_cast<R>(y) * other
        };
    }

    // Allows for bi / 2 = (x/2,y/2)
    template<typename U>
    auto operator/(const U& other) const {
        using R = std::common_type_t<T, U>;
        return TriNumber<R>{
            static_cast<R>(x) / other,
            static_cast<R>(y) / other,
        };
    }

    friend std::ostream& operator<<(std::ostream& os, const BiNumber& vec) {
        os << "(" << vec.x << ", " << vec.y << ")";
        return os;
    }
    
    std::string str() const {
        std::ostringstream oss;
        oss << *this; // Use the overloaded << operator
        return oss.str();
    }
};

/* --- Pre-defined Tri and Bi numbers --- */

// Vector/Double (64-Bit float)
typedef TriNumber<double> Vec3;
typedef BiNumber<double> Vec2;

#define VEC3_ZERO       Vec3{0.0, 0.0, 0.0}
#define VEC3_ONE        Vec3{1.0, 1.0, 1.0}
#define VEC2_ZERO       Vec2{0.0, 0.0}
#define VEC2_ONE        Vec2{1.0, 1.0}

// Float (32-Bit float)
typedef TriNumber<float> Float3;
typedef BiNumber<float> Float2;

#define FLOAT3_ZERO     Float3{0.0f, 0.0f, 0.0f}
#define FLOAT3_ONE      Float3{1.0f, 1.0f, 1.0f}
#define FLOAT2_ZERO     Float2{0.0f, 0.0f}
#define FLOAT2_ONE      Float2{1.0f, 1.0f}

// 8-Bit Integer
typedef TriNumber<int8_t> Int8_3;
typedef BiNumber<int8_t> Int8_2;

#define INT8_3_ZERO     Int8_3{0, 0, 0}
#define INT8_3_ONE      Int8_3{1, 1, 1}
#define INT8_2_ZERO     Int8_2{0, 0}
#define INT8_2_ONE      Int8_2{1, 1}

typedef Int8_3 Byte3;
typedef Int8_2 Byte2;

#define BYTE3_ZERO      INT8_3_ZERO
#define BYTE3_ONE       INT8_3_ONE
#define BYTE2_ZERO      INT8_2_ZERO
#define BYTE2_ONE       INT8_2_ONE

// 16-Bit Integer
typedef TriNumber<int16_t> Int16_3;
typedef BiNumber<int16_t> Int16_2;

#define INT16_3_ZERO    Int16_3{0, 0, 0}
#define INT16_3_ONE     Int16_3{1, 1, 1}
#define INT16_2_ZERO    Int16_2{0, 0}
#define INT16_2_ONE     Int16_2{1, 1}

typedef Int16_3 Short3;
typedef Int16_2 Short2;

#define SHORT3_ZERO     INT16_3_ZERO
#define SHORT3_ONE      INT16_3_ONE
#define SHORT2_ZERO     INT16_2_ZERO
#define SHORT2_ONE      INT16_2_ONE

// 32-Bit Integer (default)
typedef TriNumber<int32_t> Int32_3;
typedef BiNumber<int32_t> Int32_2;

#define INT32_3_ZERO    Int32_3{0, 0, 0}
#define INT32_3_ONE     Int32_3{1, 1, 1}
#define INT32_2_ZERO    Int32_2{0, 0}
#define INT32_2_ONE     Int32_2{1, 1}

typedef Int32_3 Int3;
typedef Int32_2 Int2;

#define INT3_ZERO       INT32_3_ZERO
#define INT3_ONE        INT32_3_ONE
#define INT2_ZERO       INT32_2_ZERO
#define INT2_ONE        INT32_2_ONE

// 64-Bit Integer
typedef TriNumber<int64_t> Int64_3;
typedef BiNumber<int64_t> Int64_2;

#define INT64_3_ZERO    Int64_3{0, 0, 0}
#define INT64_3_ONE     Int64_3{1, 1, 1}
#define INT64_2_ZERO    Int64_2{0, 0}
#define INT64_2_ONE     Int64_2{1, 1}

typedef Int64_3 Long3;
typedef Int64_2 Long2;

#define LONG3_ZERO      INT64_3_ZERO
#define LONG3_ONE       INT64_3_ONE
#define LONG2_ZERO      INT64_2_ZERO
#define LONG2_ONE       INT64_2_ONE