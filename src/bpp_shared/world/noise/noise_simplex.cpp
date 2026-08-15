/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "noise_simplex.h"
#include "noise_generator.h"

#include <algorithm>
#include <cassert>

NoiseSimplex::NoiseSimplex() {
	Java::Random rand;
	InitPermTable(rand);
}

NoiseSimplex::NoiseSimplex(Java::Random& _rand) {
	InitPermTable(_rand);
}

void NoiseSimplex::InitPermTable(Java::Random& _rand) {
	NoiseGenerator::InitPermTable(_rand);
	for (int32_t i = 0; i < 512; ++i)
		permMod12[i] = permutations[i] % 12;
}

void NoiseSimplex::GenerateNoise(std::span<double> _noiseField, Vec2 _offset, Int32_2 _size, Vec2 _scale,
                                 double _amplitude) {
	const int32_t width = _size.x;
	const int32_t height = _size.y;
	if (width <= 0 || height <= 0)
		return;

	const gen_float scaleX = _scale.x;
	const gen_float scaleY = _scale.y;
	const gen_float offsetX = _offset.x;
	const gen_float offsetY = _offset.y;
	const gen_float amplitude = gen_float(_amplitude * 70.0);

	double* const output = _noiseField.data();

	// Precompute everything depending only on Y.
	//
	// x0 cannot be precomputed because the simplex skew depends on x,
	// but y0/y0a/y0b can be partially derived once the x-dependent
	// skew is known.
	std::vector<gen_float> yPositions(height);

	for (int32_t y = 0; y < height; ++y) {
		yPositions[y] = (offsetY + gen_float(y)) * scaleY + coordinate.y;
	}

	for (int32_t x = 0; x < width; ++x) {
		const gen_float xPos = (offsetX + gen_float(x)) * scaleX + coordinate.x;
		double* out = output + static_cast<size_t>(x) * height;
		for (int32_t y = 0; y < height; ++y) {
			const gen_float yPos = yPositions[y];

			// Skew the input space.
			const gen_float skew = (xPos + yPos) * SKEWING;
			const int32_t x0 = Wrap(xPos + skew);
			const int32_t y0 = Wrap(yPos + skew);
			const gen_float unskewed = gen_float(x0 + y0) * UNSKEWING;
			const gen_float x0a = gen_float(x0) - unskewed;
			const gen_float y0a = gen_float(y0) - unskewed;
			const gen_float x0b = xPos - x0a;
			const gen_float y0b = yPos - y0a;
			const bool lowerRight = x0b > y0b;
			const int32_t i = lowerRight ? 1 : 0;
			const int32_t j = lowerRight ? 0 : 1;
			const gen_float x0c = x0b - gen_float(i) + UNSKEWING;
			const gen_float y0c = y0b - gen_float(j) + UNSKEWING;
			const gen_float x1c = x0b - 1.0 + 2.0 * UNSKEWING;
			const gen_float y1c = y0b - 1.0 + 2.0 * UNSKEWING;
			const int32_t xHash = x0 & 255;
			const int32_t yHash = y0 & 255;
			const uint8_t* const perm = permutations;
			const uint8_t* const mod12 = permMod12;
			const int32_t grad0 = mod12[xHash + perm[yHash]];
			const int32_t grad1 = mod12[xHash + i + perm[yHash + j]];
			const int32_t grad2 = mod12[xHash + 1 + perm[yHash + 1]];
			const gen_float contribution = Contribution(grad0, x0b, y0b) + Contribution(grad1, x0c, y0c) +
			                               Contribution(grad2, x1c, y1c);

			*out++ += double(contribution * amplitude);
		}
	}
}