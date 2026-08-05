/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "noise_simplex.h"
#include <cassert>

NoiseSimplex::NoiseSimplex() {
	Java::Random rand;
	InitPermTable(rand);
}

NoiseSimplex::NoiseSimplex(Java::Random& _rand) {
	InitPermTable(_rand);
}

void NoiseSimplex::InitPermTable(Java::Random& _rand) {
	coordinate.x = _rand.NextDouble() * 256.0;
	coordinate.y = _rand.NextDouble() * 256.0;
	coordinate.z = _rand.NextDouble() * 256.0;

	for (int32_t i = 0; i < 256; ++i) {
		permutations[i] = i;
	}

	for (int32_t i = 0; i < 256; ++i) {
		int32_t j = _rand.NextInt(256 - i) + i;
		std::swap(permutations[i], permutations[j]);
		permutations[i + 256] = permutations[i];
	}

	// Precompute the modulo once, at init time, instead of every sample.
	for (int32_t i = 0; i < 512; ++i) {
		permMod12[i] = permutations[i] % 12;
	}
}

void NoiseSimplex::GenerateNoise(std::span<double> _noiseField, Vec2 _offset, Int32_2 _size, Vec2 _scale,
                                 double _amplitude) {
	double* out = _noiseField.data();

	std::vector<double> yPositions(_size.y);
	for (int32_t yI = 0; yI < _size.y; ++yI) {
		yPositions[yI] = (_offset.y + double(yI)) * _scale.y + coordinate.y;
	}

	for (int32_t xI = 0; xI < _size.x; ++xI) {
		double xPos = (_offset.x + double(xI)) * _scale.x + coordinate.x;

		for (int32_t yI = 0; yI < _size.y; ++yI) {
			double yPos = yPositions[yI];
			double skew = (xPos + yPos) * SKEWING;
			int32_t x0 = Wrap(xPos + skew);
			int32_t y0 = Wrap(yPos + skew);
			double unskewed = double(x0 + y0) * UNSKEWING;
			double x0a = double(x0) - unskewed;
			double y0a = double(y0) - unskewed;
			double x0b = xPos - x0a;
			double y0b = yPos - y0a;
			int8_t i;
			int8_t j;
			if (x0b > y0b) {
				i = 1;
				j = 0;
			} else {
				i = 0;
				j = 1;
			}

			double x0c = x0b - double(i) + UNSKEWING;
			double y0c = y0b - double(j) + UNSKEWING;
			double x1c = x0b - 1.0 + 2.0 * UNSKEWING;
			double y1c = y0b - 1.0 + 2.0 * UNSKEWING;
			int32_t xInt = x0 & 255;
			int32_t yInt = y0 & 255;
			int32_t grad0 = permMod12[xInt + permutations[yInt]];
			int32_t grad1 = permMod12[xInt + i + permutations[yInt + j]];
			int32_t grad2 = permMod12[xInt + 1 + permutations[yInt + 1]];
			double term0 = 0.5 - x0b * x0b - y0b * y0b;
			double contrib0;
			if (term0 < 0.0) {
				contrib0 = 0.0;
			} else {
				term0 *= term0;
				contrib0 = term0 * term0 * DotProd(GRADIENTS[grad0], x0b, y0b);
			}

			double term1 = 0.5 - x0c * x0c - y0c * y0c;
			double contrib1;
			if (term1 < 0.0) {
				contrib1 = 0.0;
			} else {
				term1 *= term1;
				contrib1 = term1 * term1 * DotProd(GRADIENTS[grad1], x0c, y0c);
			}

			double term2 = 0.5 - x1c * x1c - y1c * y1c;
			double contrib2;
			if (term2 < 0.0) {
				contrib2 = 0.0;
			} else {
				term2 *= term2;
				contrib2 = term2 * term2 * DotProd(GRADIENTS[grad2], x1c, y1c);
			}

			*out++ += 70.0 * (contrib0 + contrib1 + contrib2) * _amplitude;
		}
	}
}