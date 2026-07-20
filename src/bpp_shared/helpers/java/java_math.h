/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

// Library for emulating Java/Java Edition math functions

/**
 * @brief Linear interpolation function
 * 
 * @param t Interpolation factor
 * @param a Start value (t = 0.0)
 * @param b End value (t = 1.0)
 * @return Interpolated value between a and b
 */
inline double Lerp(double _t, double _a, double _b) {
	return _a + _t * (_b - _a);
}

/**
 * @brief 3D Perlin noise gradient function
 * 
 * @param hash Hashed lattice value
 * @param x X of Distance Vector
 * @param y Y of Distance Vector
 * @param z Z of Distance Vector
 * @return double 
 */
inline double Grad3d(int32_t _hash, double _x, double _y, double _z) {
	_hash &= 15;
	double u = _hash < 8 ? _x : _y;
	double v = _hash < 4 ? _y : (_hash != 12 && _hash != 14 ? _z : _x);
	return ((_hash & 1) == 0 ? u : -u) + ((_hash & 2) == 0 ? v : -v);
}

/**
 * @brief 2D Perlin noise gradient function
 * 
 * @param hash Hashed lattice value
 * @param x X of Distance Vector
 * @param y Y of Distance Vector
 * @return double 
 */
inline double Grad2d(int32_t _hash, double _x, double _y) {
	_hash &= 15;
	double u = double(1 - ((_hash & 8) >> 3)) * _x;
	double v = _hash < 4 ? 0.0 : (_hash != 12 && _hash != 14 ? _y : _x);
	return ((_hash & 1) == 0 ? u : -u) + ((_hash & 2) == 0 ? v : -v);
}

/**
 * @brief Perlin-noise easing function
 * 
 * @param value Input value
 * @return Eased output value 
 */
inline double Fade(double _value) {
	return _value * _value * _value * (_value * (_value * 6.0 - 15.0) + 10.0);
}

/**
 * @brief Java-equivalent functions
 * 
 */
namespace Java {
// The following should be somewhat faithful implementation of
// Java's casting functions, as defined in
// "Chapter 5. Conversions and Contexts"
/**
	 * @brief Casts a double to a 64-bit integer
	 */
inline int64_t DoubleToInt64(double _value) {
	if (std::isnan(_value))
		return 0;
	if (_value > double(INT64_MAX))
		return INT64_MAX;
	if (_value < double(INT64_MIN))
		return INT64_MIN;
	if (_value > 0)
		return int64_t(std::floor(_value));
	if (_value < 0)
		return int64_t(std::ceil(_value));
	return 0;
}
/**
	 * @brief Casts a double to a 32-bit integer
	 */
inline int32_t DoubleToInt32(double _value) {
	if (std::isnan(_value))
		return 0;
	if (_value > double(INT32_MAX))
		return INT32_MAX;
	if (_value < double(INT32_MIN))
		return INT32_MIN;
	if (_value > 0)
		return int32_t(std::floor(_value));
	if (_value < 0)
		return int32_t(std::ceil(_value));
	return 0;
}
/**
	 * @brief Casts a float to a 64-bit integer
	 */
inline int64_t FloatToInt64(float _value) {
	if (std::isnan(_value))
		return 0;
	if (_value > float(INT64_MAX))
		return INT64_MAX;
	if (_value < float(INT64_MIN))
		return INT64_MIN;
	if (_value > 0)
		return int64_t(std::floor(_value));
	if (_value < 0)
		return int64_t(std::ceil(_value));
	return 0;
}
/**
	 * @brief Casts a float to a 32-bit integer
	 */
inline int32_t FloatToInt32(float _value) {
	if (std::isnan(_value))
		return 0;
	if (_value > float(INT32_MAX))
		return INT32_MAX;
	if (_value < float(INT32_MIN))
		return INT32_MIN;
	if (_value > 0)
		return int32_t(std::floor(_value));
	if (_value < 0)
		return int32_t(std::ceil(_value));
	return 0;
}
}; // namespace Java

/**
* @brief Java-equivalent hashing function
* 
* @param value The input string
* @return Hashed string expressed as an integer
*/
inline int32_t HashCode(std::string _value) {
	int32_t h = 0;
	if (h == 0 && _value.size() > 0) {
		for (size_t i = 0; i < _value.size(); i++) {
			h = 31 * h + _value[i];
		}
	}
	return h;
}

/**
 * @brief A struct that's used like Javas Math.java library
 * 
 */
struct JavaMath {
	static constexpr double PI = 3.141592653589793;
	static constexpr float PI_FLOAT = float(PI);
	static int32_t Abs(int32_t _a) {
		return (_a < 0) ? -_a : _a;
	}
};

/**
 * @brief A small helper that's used to simplify or speed up some code
 * 
 */
struct MathHelper {
	static constexpr size_t TABLE_SIZE = 65536;
	// Requires C++17
	inline static std::array<float, TABLE_SIZE> m_SIN_TABLE{};

	static float Sin(float _x) {
		return m_SIN_TABLE[Java::FloatToInt32(_x * 10430.378f) & 0xFFFF];
	}

	static float Cos(float _x) {
		return m_SIN_TABLE[(Java::FloatToInt32(_x * 10430.378f + 16384.0f)) & 0xFFFF];
	}

	static float SqrtFloat(float _x) {
		return std::sqrt(_x);
	}

	static float SqrtDouble(double _x) {
		return static_cast<float>(std::sqrt(_x));
	}

	static int32_t FloorFloat(float _x) {
		int32_t i = Java::FloatToInt32(_x);
		return _x < static_cast<float>(i) ? i - 1 : i;
	}

	static int32_t FloorDouble(double _x) {
		int32_t i = Java::DoubleToInt32(_x);
		return _x < static_cast<double>(i) ? i - 1 : i;
	}

	static float Abs(float _x) {
		return _x >= 0.0f ? _x : -_x;
	}

	static double AbsMax(double _a, double _b) {
		if (_a < 0.0)
			_a = -_a;
		if (_b < 0.0)
			_b = -_b;
		return _a > _b ? _a : _b;
	}

	static inline void InitSinTable() {
		for (size_t i = 0; i < MathHelper::TABLE_SIZE; ++i)
			MathHelper::m_SIN_TABLE[i] = float(std::sin(double(i) * JavaMath::PI * 2.0 / double(MathHelper::TABLE_SIZE)));
	}
};
