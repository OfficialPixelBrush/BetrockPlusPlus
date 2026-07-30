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
constexpr inline double Lerp(const double _t, const double _a, const double _b) {
	return _a + _t * (_b - _a);
}

// Precomputed tables for easier Gradient functions
static constexpr std::array<int8_t, 16> K_GRAD3_U = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 };
static constexpr std::array<int8_t, 16> K_GRAD3_V = { 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 0, 2 };
static constexpr std::array<int8_t, 16> K_GRAD2_U = { 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2 };
static constexpr std::array<int8_t, 16> K_GRAD2_V = { 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1 };
static constexpr std::array<double, 16> K_SIGN_BIT0 = { 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1 };
static constexpr std::array<double, 16> K_SIGN_BIT1 = { 1, 1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1 };

/**
 * @brief 3D Perlin noise gradient function
 * 
 * @param hash Hashed lattice value
 * @param x X of Distance Vector
 * @param y Y of Distance Vector
 * @param z Z of Distance Vector
 * @return double 
 */
constexpr inline double Grad3d(int32_t _hash, const double _x, const double _y, const double _z) {
	const uint32_t h = static_cast<uint32_t>(_hash) & 15u;
	const double comp[3] = { _x, _y, _z };
	const double u = comp[K_GRAD3_U[h]];
	const double v = comp[K_GRAD3_V[h]];
	return u * K_SIGN_BIT0[h] + v * K_SIGN_BIT1[h];
}
/**
 * @brief 2D Perlin noise gradient function
 * 
 * @param hash Hashed lattice value
 * @param x X of Distance Vector
 * @param y Y of Distance Vector
 * @return double 
 */
constexpr inline double Grad2d(int32_t _hash, const double _x, const double _y) {
	const uint32_t h = static_cast<uint32_t>(_hash) & 15u;
	const double comp[3] = { _x, _y, 0.0 }; // index 2 = "zero" slot
	const double u = comp[K_GRAD2_U[h]];
	const double v = comp[K_GRAD2_V[h]];
	return u * K_SIGN_BIT0[h] + v * K_SIGN_BIT1[h];
}

/**
 * @brief Perlin-noise easing function
 * 
 * @param value Input value
 * @return Eased output value 
 */
constexpr inline double Fade(const double _value) {
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
constexpr inline int64_t DoubleToInt64(const double _value) {
	if (_value > 0)
		return int64_t(std::floor(_value));
	if (_value < 0)
		return int64_t(std::ceil(_value));
	if (_value > double(INT64_MAX)) [[unlikely]]
		return INT64_MAX;
	if (_value < double(INT64_MIN)) [[unlikely]]
		return INT64_MIN;
	return 0;
}
/**
	 * @brief Casts a double to a 32-bit integer
	 */
constexpr inline int32_t DoubleToInt32(const double _value) {
	if (_value > 0)
		return int32_t(std::floor(_value));
	if (_value < 0)
		return int32_t(std::ceil(_value));
	if (_value > double(INT32_MAX)) [[unlikely]]
		return INT32_MAX;
	if (_value < double(INT32_MIN)) [[unlikely]]
		return INT32_MIN;
	return 0;
}
/**
	 * @brief Casts a float to a 64-bit integer
	 */
constexpr inline int64_t FloatToInt64(const float _value) {
	if (_value > 0)
		return int64_t(std::floor(_value));
	if (_value < 0)
		return int64_t(std::ceil(_value));
	if (_value > float(INT64_MAX)) [[unlikely]]
		return INT64_MAX;
	if (_value < float(INT64_MIN)) [[unlikely]]
		return INT64_MIN;
	return 0;
}
/**
	 * @brief Casts a float to a 32-bit integer
	 */
constexpr inline int32_t FloatToInt32(const float _value) {
	if (_value > 0)
		return int32_t(std::floor(_value));
	if (_value < 0)
		return int32_t(std::ceil(_value));
	if (_value > float(INT32_MAX)) [[unlikely]]
		return INT32_MAX;
	if (_value < float(INT32_MIN)) [[unlikely]]
		return INT32_MIN;
	return 0;
}
}; // namespace Java

/**
* @brief Java-equivalent hashing function
* 
* @param value The input string
* @return Hashed string expressed as an integer
*/
inline int32_t HashCode(const std::string _value) {
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
	static constexpr inline int32_t Abs(int32_t _a) {
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

	static constexpr inline float Sin(float _x) {
		return m_SIN_TABLE[Java::FloatToInt32(_x * 10430.378f) & 0xFFFF];
	}

	static constexpr inline float Cos(float _x) {
		return m_SIN_TABLE[(Java::FloatToInt32(_x * 10430.378f + 16384.0f)) & 0xFFFF];
	}

	static constexpr inline float SqrtFloat(float _x) {
		return std::sqrt(_x);
	}

	static constexpr inline float SqrtDouble(double _x) {
		return static_cast<float>(std::sqrt(_x));
	}

	static constexpr inline int32_t FloorFloat(float _x) {
		int32_t i = Java::FloatToInt32(_x);
		return _x < static_cast<float>(i) ? i - 1 : i;
	}

	static constexpr inline int32_t FloorDouble(double _x) {
		int32_t i = Java::DoubleToInt32(_x);
		return _x < static_cast<double>(i) ? i - 1 : i;
	}

	static constexpr inline float Abs(float _x) {
		return _x >= 0.0f ? _x : -_x;
	}

	static constexpr inline double AbsMax(double _a, double _b) {
		if (_a < 0.0)
			_a = -_a;
		if (_b < 0.0)
			_b = -_b;
		return _a > _b ? _a : _b;
	}

	static inline void InitSinTable() {
		for (size_t i = 0; i < MathHelper::TABLE_SIZE; ++i)
			MathHelper::m_SIN_TABLE[i] = float(
			    std::sin(double(i) * JavaMath::PI * 2.0 / double(MathHelper::TABLE_SIZE)));
	}
};
