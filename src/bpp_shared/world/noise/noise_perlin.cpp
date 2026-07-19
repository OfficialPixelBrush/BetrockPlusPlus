/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "noise_perlin.h"
#include "java_math.h"

NoisePerlin::NoisePerlin() {
	Java::Random rand = Java::Random();
	InitPermTable(rand);
}

/**
 * @brief Construct a new Noise Perlin object
 * 
 * @param rand The random number generator that should be used
 */
NoisePerlin::NoisePerlin(Java::Random& rand) {
	InitPermTable(rand);
}

void NoisePerlin::InitPermTable(Java::Random& rand) {
	m_coordinate.m_x = rand.nextDouble() * 256.0;
	m_coordinate.m_y = rand.nextDouble() * 256.0;
	m_coordinate.m_z = rand.nextDouble() * 256.0;

	for (int32_t i = 0; i < 256; ++i) {
		m_permutations[i] = i;
	}

	for (int32_t i = 0; i < 256; ++i) {
		int32_t j = rand.nextInt(256 - i) + i;
		std::swap(m_permutations[i], m_permutations[j]);
		m_permutations[i + 256] = m_permutations[i];
	}
}

/**
 * @brief This is a rather standard implementation of "Improved Perlin Noise",
 *        as described by Ken Perlin in 2002
 *        This version is mainly used by the infdev generator
 *        but Beta still implements and uses it for some things,
 *        namely the nether
 * 
 * @param pos Coordinate at which to sample the noise
 * @return Noise value
 */
double NoisePerlin::GenerateNoiseBase(Vec3 pos) {
	pos.m_x += m_coordinate.m_x;
	pos.m_y += m_coordinate.m_y;
	pos.m_z += m_coordinate.m_z;
	// The farlands are caused by this getting cast to a 32-Bit Integer.
	// Change these int32_t to int64_t to fix the farlands in Infdev
	int32_t xInt = Java::DoubleToInt32(pos.m_x);
	int32_t yInt = Java::DoubleToInt32(pos.m_y);
	int32_t zInt = Java::DoubleToInt32(pos.m_z);
	if (pos.m_x < double(xInt))
		--xInt;
	if (pos.m_y < double(yInt))
		--yInt;
	if (pos.m_z < double(zInt))
		--zInt;

	int32_t xIndex = xInt & 255;
	int32_t yIndex = yInt & 255;
	int32_t zIndex = zInt & 255;

	pos.m_x -= double(xInt);
	pos.m_y -= double(yInt);
	pos.m_z -= double(zInt);
	double w = fade(pos.m_x);
	double v = fade(pos.m_y);
	double u = fade(pos.m_z);
	int32_t permXY = m_permutations[xIndex] + yIndex;
	int32_t permXYZ = m_permutations[permXY] + zIndex;
	// Some of the following code is weird,
	// probably because it got optimized by Java to use
	// fewer variables or Notch did this to be efficient
	permXY = m_permutations[permXY + 1] + zIndex;
	xIndex = m_permutations[xIndex + 1] + yIndex;
	yIndex = m_permutations[xIndex] + zIndex;
	xIndex = m_permutations[xIndex + 1] + zIndex;
	return lerp(u,
	            lerp(v,
	                 lerp(w, grad3d(m_permutations[permXYZ], pos.m_x, pos.m_y, pos.m_z),
	                      grad3d(m_permutations[yIndex], pos.m_x - 1.0, pos.m_y, pos.m_z)),
	                 lerp(w, grad3d(m_permutations[permXY], pos.m_x, pos.m_y - 1.0, pos.m_z),
	                      grad3d(m_permutations[xIndex], pos.m_x - 1.0, pos.m_y - 1.0, pos.m_z))),
	            lerp(v,
	                 lerp(w, grad3d(m_permutations[permXYZ + 1], pos.m_x, pos.m_y, pos.m_z - 1.0),
	                      grad3d(m_permutations[yIndex + 1], pos.m_x - 1.0, pos.m_y, pos.m_z - 1.0)),
	                 lerp(w, grad3d(m_permutations[permXY + 1], pos.m_x, pos.m_y - 1.0, pos.m_z - 1.0),
	                      grad3d(m_permutations[xIndex + 1], pos.m_x - 1.0, pos.m_y - 1.0, pos.m_z - 1.0))));
}

double NoisePerlin::GenerateNoise(Vec2 coord) {
	return GenerateNoiseBase(Vec3{ coord.m_x, coord.m_y, 0.0 });
}

double NoisePerlin::GenerateNoise(Vec3 coord) {
	return GenerateNoiseBase(coord);
}

/**
 * @brief The main noise generator employed by the Beta 1.7.3 world generator
 * 
 * @param noiseField the vector the noise will be written to
 * @param offset The positional offset within the perlin noise that'll be rendered
 * @param size The size of the volume that'll be saved the noise field
 * @param scale The scale of the perlin noise equation
 * @param amplitude The amplitude multiplier of the perlin noise function
 */
void NoisePerlin::GenerateNoise(std::vector<double>& noiseField, Vec3 offset, Int3 size, Vec3 scale, double amplitude) {
	if (size.m_y == 1) {
		size_t index = 0;
		double invAmp = 1.0 / amplitude;

		for (int32_t x = 0; x < size.m_x; ++x) {
			double fx = (offset.m_x + x) * scale.m_x + m_coordinate.m_x;
			int32_t ix = Java::DoubleToInt32(fx);
			if (fx < ix)
				--ix;
			int32_t px = ix & 255;
			fx -= ix;
			double u = fade(fx);

			for (int32_t z = 0; z < size.m_z; ++z) {
				double fz = (offset.m_z + z) * scale.m_z + m_coordinate.m_z;
				int32_t iz = Java::DoubleToInt32(fz);
				if (fz < iz)
					--iz;
				int32_t pz = iz & 255;
				fz -= iz;
				double w = fade(fz);

				int32_t a = m_permutations[px] + 0;
				int32_t aa = m_permutations[a] + pz;
				int32_t b = m_permutations[px + 1] + 0;
				int32_t ba = m_permutations[b] + pz;

				double x1 = lerp(u, grad2d(m_permutations[aa], fx, fz), grad3d(m_permutations[ba], fx - 1.0, 0.0, fz));

				double x2 = lerp(u, grad3d(m_permutations[aa + 1], fx, 0.0, fz - 1.0),
				                 grad3d(m_permutations[ba + 1], fx - 1.0, 0.0, fz - 1.0));

				double result = lerp(w, x1, x2);
				noiseField[index++] += result * invAmp;
			}
		}
	} else {
		size_t index = 0;
		double invAmp = 1.0 / amplitude;
		int32_t lastPermY = -1;

		double lerpAX = 0.0, lerpBX = 0.0;
		double lerpAY = 0.0, lerpBY = 0.0;

		for (int32_t x = 0; x < size.m_x; ++x) {
			double fx = (offset.m_x + x) * scale.m_x + m_coordinate.m_x;
			int32_t ix = Java::DoubleToInt32(fx);
			if (fx < ix)
				--ix;
			int32_t px = ix & 255;
			fx -= ix;
			double u = fade(fx);

			for (int32_t z = 0; z < size.m_z; ++z) {
				double fz = (offset.m_z + z) * scale.m_z + m_coordinate.m_z;
				int32_t iz = Java::DoubleToInt32(fz);
				if (fz < iz)
					--iz;
				int32_t pz = iz & 255;
				fz -= iz;
				double w = fade(fz);

				for (int32_t y = 0; y < size.m_y; ++y) {
					double fy = (offset.m_y + y) * scale.m_y + m_coordinate.m_y;
					int32_t iy = Java::DoubleToInt32(fy);
					if (fy < iy)
						--iy;
					int32_t py = iy & 255;
					fy -= iy;
					double v = fade(fy);

					if (y == 0 || py != lastPermY) {
						lastPermY = py;

						int32_t A = m_permutations[px] + py;
						int32_t AA = m_permutations[A] + pz;
						int32_t AB = m_permutations[A + 1] + pz;
						int32_t B = m_permutations[px + 1] + py;
						int32_t BA = m_permutations[B] + pz;
						int32_t BB = m_permutations[B + 1] + pz;

						lerpAX = lerp(u, grad3d(m_permutations[AA], fx, fy, fz),
						              grad3d(m_permutations[BA], fx - 1.0, fy, fz));

						lerpBX = lerp(u, grad3d(m_permutations[AB], fx, fy - 1.0, fz),
						              grad3d(m_permutations[BB], fx - 1.0, fy - 1.0, fz));

						lerpAY = lerp(u, grad3d(m_permutations[AA + 1], fx, fy, fz - 1.0),
						              grad3d(m_permutations[BA + 1], fx - 1.0, fy, fz - 1.0));

						lerpBY = lerp(u, grad3d(m_permutations[AB + 1], fx, fy - 1.0, fz - 1.0),
						              grad3d(m_permutations[BB + 1], fx - 1.0, fy - 1.0, fz - 1.0));
					}

					double i1 = lerp(v, lerpAX, lerpBX);
					double i2 = lerp(v, lerpAY, lerpBY);
					double result = lerp(w, i1, i2);

					noiseField[index++] += result * invAmp;
				}
			}
		}
	}
}