/*
    BetrockPlusPlus
    Copyright (C) 2026  PixelBrushArt and JcbbcEnjoyer

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    This header file is responsible for all numeric structs,
    such as Int3 and similar that make handling of the code-base easier.
*/

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

// Vector/Double
typedef TriNumber<double> Vec3;
typedef BiNumber<double> Vec2;

#define VEC3_ZERO       Vec3{0.0, 0.0, 0.0}
#define VEC3_ONE        Vec3{1.0, 1.0, 1.0}
#define VEC2_ZERO       Vec2{0.0, 0.0}
#define VEC2_ONE        Vec2{1.0, 1.0}

// Float
typedef TriNumber<float> Float3;
typedef BiNumber<float> Float2;

#define FLOAT3_ZERO     Float3{0.0f, 0.0f, 0.0f}
#define FLOAT3_ONE      Float3{1.0f, 1.0f, 1.0f}
#define FLOAT2_ZERO     Float2{0.0f, 0.0f}
#define FLOAT2_ONE      Float2{1.0f, 1.0f}

// 8-Bit Integer
typedef TriNumber<int8_t> Int3_8;
typedef BiNumber<int8_t> Int2_8;

#define INT3_8_ZERO     Int3_8{0, 0, 0}
#define INT3_8_ONE      Int3_8{1, 1, 1}
#define INT2_8_ZERO     Int2_8{0, 0}
#define INT2_8_ONE      Int2_8{1, 1}

// 16-Bit Integer
typedef TriNumber<int16_t> Int3_16;
typedef BiNumber<int16_t> Int2_16;

#define INT3_16_ZERO    Int3_16{0, 0, 0}
#define INT3_16_ONE     Int3_16{1, 1, 1}
#define INT2_16_ZERO    Int2_16{0, 0}
#define INT2_16_ONE     Int2_16{1, 1}

// 32-Bit Integer (default)
typedef TriNumber<int32_t> Int3_32;
typedef BiNumber<int32_t> Int2_32;
typedef Int3_32 Int3;
typedef Int2_32 Int2;

#define INT3_32_ZERO    Int3_32{0, 0, 0}
#define INT3_32_ONE     Int3_32{1, 1, 1}
#define INT2_32_ZERO    Int2_32{0, 0}
#define INT2_32_ONE     Int2_32{1, 1}

#define INT3_ZERO       INT3_32_ZERO
#define INT3_ONE        INT3_32_ONE
#define INT2_ZERO       INT2_32_ZERO
#define INT2_ONE        INT2_32_ONE

// 64-Bit Integer
typedef TriNumber<int64_t> Int3_64;
typedef BiNumber<int64_t> Int2_64;

#define INT3_64_ZERO    Int3_64{0, 0, 0}
#define INT3_64_ONE     Int3_64{1, 1, 1}
#define INT2_64_ZERO    Int2_64{0, 0}
#define INT2_64_ONE     Int2_64{1, 1}