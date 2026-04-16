/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
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
inline double lerp(double t, double a, double b) {
	return a + t * (b - a);
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
inline double grad3d(int32_t hash, double x, double y, double z) {
	hash &= 15;
	double u = hash < 8 ? x : y;
	double v = hash < 4 ? y : (hash != 12 && hash != 14 ? z : x);
	return ((hash & 1) == 0 ? u : -u) + ((hash & 2) == 0 ? v : -v);
}

/**
 * @brief 2D Perlin noise gradient function
 * 
 * @param hash Hashed lattice value
 * @param x X of Distance Vector
 * @param y Y of Distance Vector
 * @return double 
 */
inline double grad2d(int32_t hash, double x, double y) {
	hash &= 15;
	double u = double(1 - ((hash & 8) >> 3)) * x;
	double v = hash < 4 ? 0.0 : (hash != 12 && hash != 14 ? y : x);
	return ((hash & 1) == 0 ? u : -u) + ((hash & 2) == 0 ? v : -v);
}

/**
 * @brief Perlin-noise easing function
 * 
 * @param value Input value
 * @return Eased output value 
 */
inline double fade(double value) { return value * value * value * (value * (value * 6.0 - 15.0) + 10.0); }

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
	static int64_t DoubleToInt64(double value) {
		if (std::isnan(value))
			return 0;
		if (value > double(INT64_MAX))
			return INT64_MAX;
		if (value < double(INT64_MIN))
			return INT64_MIN;
		if (value > 0)
			return int64_t(std::floor(value));
		if (value < 0)
			return int64_t(std::ceil(value));
		return 0;
	}
	/**
	 * @brief Casts a double to a 32-bit integer
	 */
	static int32_t DoubleToInt32(double value) {
		if (std::isnan(value))
			return 0;
		if (value > double(INT32_MAX))
			return INT32_MAX;
		if (value < double(INT32_MIN))
			return INT32_MIN;
		if (value > 0)
			return int32_t(std::floor(value));
		if (value < 0)
			return int32_t(std::ceil(value));
		return 0;
	}
	/**
	 * @brief Casts a float to a 64-bit integer
	 */
	static int64_t FloatToInt64(float value) {
		if (std::isnan(value))
			return 0;
		if (value > float(INT64_MAX))
			return INT64_MAX;
		if (value < float(INT64_MIN))
			return INT64_MIN;
		if (value > 0)
			return int64_t(std::floor(value));
		if (value < 0)
			return int64_t(std::ceil(value));
		return 0;
	}
	/**
	 * @brief Casts a float to a 32-bit integer
	 */
	static int32_t FloatToInt32(float value) {
		if (std::isnan(value))
			return 0;
		if (value > float(INT32_MAX))
			return INT32_MAX;
		if (value < float(INT32_MIN))
			return INT32_MIN;
		if (value > 0)
			return int32_t(std::floor(value));
		if (value < 0)
			return int32_t(std::ceil(value));
		return 0;
	}
};

/**
* @brief Java-equivalent hashing function
* 
* @param value The input string
* @return Hashed string expressed as an integer
*/
inline int32_t hashCode(std::string value) {
	int32_t h = 0;
	if (h == 0 && value.size() > 0) {
		for (size_t i = 0; i < value.size(); i++) {
			h = 31 * h + value[i];
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
	static int32_t abs(int32_t a) { return (a < 0) ? -a : a; }
};

/**
 * @brief A small helper that's used to simplify or speed up some code
 * 
 */
struct MathHelper {
	static constexpr size_t TABLE_SIZE = 65536;
	static std::array<float, TABLE_SIZE> SIN_TABLE;

	static float sin(float x) { return SIN_TABLE[Java::FloatToInt32(x * 10430.378f) & 0xFFFF]; }

	static float cos(float x) { return SIN_TABLE[(Java::FloatToInt32(x * 10430.378f + 16384.0f)) & 0xFFFF]; }

	static float sqrt_float(float x) { return std::sqrt(x); }

	static float sqrt_double(double x) { return static_cast<float>(std::sqrt(x)); }

	static int32_t floor_float(float x) {
		int32_t i = Java::FloatToInt32(x);
		return x < static_cast<float>(i) ? i - 1 : i;
	}

	static int32_t floor_double(double x) {
		int32_t i = Java::DoubleToInt32(x);
		return x < static_cast<double>(i) ? i - 1 : i;
	}
	
	static float abs(float x) { return x >= 0.0f ? x : -x; }

	static double abs_max(double a, double b) {
		if (a < 0.0)
			a = -a;
		if (b < 0.0)
			b = -b;
		return a > b ? a : b;
	}
};

// initialize lookup table
inline std::array<float, MathHelper::TABLE_SIZE> MathHelper::SIN_TABLE = [] {
	std::array<float, MathHelper::TABLE_SIZE> table{};
	for (size_t i = 0; i < MathHelper::TABLE_SIZE; ++i)
		table[i] = std::sin(float(i) * float(JavaMath::PI) * 2.0f / MathHelper::TABLE_SIZE);
	return table;
}();