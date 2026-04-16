/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "noise_simplex.h"

NoiseSimplex::NoiseSimplex() {
	Java::Random rand;
	InitPermTable(rand);
}

NoiseSimplex::NoiseSimplex(Java::Random& rand) {
	InitPermTable(rand);
}

void NoiseSimplex::InitPermTable(Java::Random& rand) {
	coordinate.x = rand.nextDouble() * 256.0;
	coordinate.y = rand.nextDouble() * 256.0;
	coordinate.z = rand.nextDouble() * 256.0;

	for (int32_t i = 0; i < 256; ++i) {
		permutations[i] = i;
	}

	for (int32_t i = 0; i < 256; ++i) {
		int32_t j = rand.nextInt(256 - i) + i;
		std::swap(permutations[i], permutations[j]);
		permutations[i + 256] = permutations[i];
	}
}

void NoiseSimplex::GenerateNoise(std::vector<double> &noiseField, Vec2 offset, Int32_2 size, Vec2 scale, double amplitude) {
	size_t index = 0;

	for (int32_t xI = 0; xI < size.x; ++xI) {
		double xPos = (offset.x + double(xI)) * scale.x + coordinate.x;

		for (int32_t yI = 0; yI < size.y; ++yI) {
			double yPos = (offset.y + double(yI)) * scale.y + coordinate.y;
			double skew = (xPos + yPos) * skewing;
			int32_t x0 = wrap(xPos + skew);
			int32_t y0 = wrap(yPos + skew);
			double unskewed = double(x0 + y0) * unskewing;
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

			double x0c = x0b - double(i) + unskewing;
			double y0c = y0b - double(j) + unskewing;
			double x1c = x0b - 1.0 + 2.0 * unskewing;
			double y1c = y0b - 1.0 + 2.0 * unskewing;
			int32_t xInt = x0 & 255;
			int32_t yInt = y0 & 255;
			int32_t grad0 = permutations[xInt + permutations[yInt]] % 12;
			int32_t grad1 = permutations[xInt + i + permutations[yInt + j]] % 12;
			int32_t grad2 = permutations[xInt + 1 + permutations[yInt + 1]] % 12;
			double term0 = 0.5 - x0b * x0b - y0b * y0b;
			double contrib0;
			if (term0 < 0.0) {
				contrib0 = 0.0;
			} else {
				term0 *= term0;
				contrib0 = term0 * term0 * dotProd(gradients[grad0], x0b, y0b);
			}

			double term1 = 0.5 - x0c * x0c - y0c * y0c;
			double contrib1;
			if (term1 < 0.0) {
				contrib1 = 0.0;
			} else {
				term1 *= term1;
				contrib1 = term1 * term1 * dotProd(gradients[grad1], x0c, y0c);
			}

			double term2 = 0.5 - x1c * x1c - y1c * y1c;
			double contrib2;
			if (term2 < 0.0) {
				contrib2 = 0.0;
			} else {
				term2 *= term2;
				contrib2 = term2 * term2 * dotProd(gradients[grad2], x1c, y1c);
			}

			noiseField[index++] += 70.0 * (contrib0 + contrib1 + contrib2) * amplitude;
		}
	}
}