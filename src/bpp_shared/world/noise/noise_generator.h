/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include "java_random.h"
#include "numeric_structs.h"

#ifndef REDUCED_GENERATION_PRECISION
typedef float gen_float;
#define GenFloatToInt32 Java::FloatToInt32
#else
typedef double gen_float;
#define GenFloatToInt32 Java::DoubleToInt32
#endif

/**
 * @brief The base Noise generator object that splits into Perlin and Simplex noise
 * 
 */
class NoiseGenerator {
protected:
	int32_t permutations[512];
	Vec3 coordinate;
	double GenerateNoiseBase(Vec3 _pOffset);
	void InitPermTable(Java::Random& _rand);

public:
	NoiseGenerator();
	NoiseGenerator(Java::Random& _rand);

	virtual ~NoiseGenerator() = default;
};

// Java Math functions that're only used by the generator

// Precomputed tables for easier Gradient functions
static constexpr std::array<int8_t, 16> K_GRAD3_U = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 };
static constexpr std::array<int8_t, 16> K_GRAD3_V = { 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 0, 2 };
static constexpr std::array<int8_t, 16> K_GRAD2_U = { 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2 };
static constexpr std::array<int8_t, 16> K_GRAD2_V = { 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1 };
static constexpr std::array<float, 16> K_SIGN_BIT0 = { 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1 };
static constexpr std::array<float, 16> K_SIGN_BIT1 = { 1, 1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1 };

/**
 * @brief 3D Perlin noise gradient function
 * 
 * @param hash Hashed lattice value
 * @param x X of Distance Vector
 * @param y Y of Distance Vector
 * @param z Z of Distance Vector
 * @return double 
 */
constexpr inline gen_float Grad3d(int32_t _hash, const gen_float _x, const gen_float _y, const gen_float _z) {
	const uint32_t h = static_cast<uint32_t>(_hash) & 15u;
	const gen_float comp[3] = { _x, _y, _z };
	const gen_float u = comp[K_GRAD3_U[h]];
	const gen_float v = comp[K_GRAD3_V[h]];
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
constexpr inline gen_float Grad2d(int32_t _hash, const gen_float _x, const gen_float _y) {
	const uint32_t h = static_cast<uint32_t>(_hash) & 15u;
	const gen_float comp[3] = { _x, _y, 0.0 };
	const gen_float u = comp[K_GRAD2_U[h]];
	const gen_float v = comp[K_GRAD2_V[h]];
	return u * K_SIGN_BIT0[h] + v * K_SIGN_BIT1[h];
}

/**
 * @brief Perlin-noise easing function
 * 
 * @param value Input value
 * @return Eased output value 
 */
constexpr inline gen_float Fade(const gen_float _value) {
	return _value * _value * _value * (_value * (_value * 6.0 - 15.0) + 10.0);
}