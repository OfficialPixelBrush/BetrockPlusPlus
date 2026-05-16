/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "cave_gen.h"
#include "java_math.h"

CaveGenerator::CaveGenerator() {}

/**
 * @brief Attempts to generate a cave in the current chunk
 *
 * @param chunk The chunk to carve caves into
 * @param seed  The world seed
 */
void CaveGenerator::GenerateCavesForChunk(Chunk& chunk, int64_t seed) {
	int32_t carveExtent = this->carveExtentLimit;
	this->rand.setSeed(seed);
	// Ensure odd offset
	int64_t xOffset = this->rand.nextLong() / 2L * 2L + 1L;
	int64_t zOffset = this->rand.nextLong() / 2L * 2L + 1L;

	for (int32_t cXoffset = chunk.cpos.x - carveExtent; cXoffset <= chunk.cpos.x + carveExtent; ++cXoffset) {
		for (int32_t cZoffset = chunk.cpos.z - carveExtent; cZoffset <= chunk.cpos.z + carveExtent; ++cZoffset) {
			this->rand.setSeed(((int64_t(cXoffset) * xOffset) + (int64_t(cZoffset) * zOffset)) ^ seed);
			this->GenerateCaves(chunk, { cXoffset, cZoffset });
		}
	}
}

void CaveGenerator::GenerateCaves(Chunk& chunk, Int2 chunkOffset) {
	int32_t numberOfCaves = this->rand.nextInt(this->rand.nextInt(this->rand.nextInt(40) + 1) + 1);
	if (this->rand.nextInt(15) != 0) {
		numberOfCaves = 0;
	}

	for (int32_t caveIndex = 0; caveIndex < numberOfCaves; ++caveIndex) {
		Vec3 offset = VEC3_ZERO;
		offset.x = double(chunkOffset.x * CHUNK_WIDTH + this->rand.nextInt(CHUNK_WIDTH));
		offset.y = double(this->rand.nextInt(this->rand.nextInt(120) + 8));
		offset.z = double(chunkOffset.y * CHUNK_WIDTH + this->rand.nextInt(CHUNK_WIDTH));
		int32_t numberOfNodes = 1;
		if (this->rand.nextInt(4) == 0) {
			this->CarveCave(chunk, offset);
			numberOfNodes += this->rand.nextInt(4);
		}

		for (int32_t nodeIndex = 0; nodeIndex < numberOfNodes; ++nodeIndex) {
			float carveYaw = this->rand.nextFloat() * JavaMath::PI_FLOAT * 2.0f;
			float carvePitch = (this->rand.nextFloat() - 0.5f) * 2.0f / 8.0f;
			float tunnelRadius = this->rand.nextFloat() * 2.0f + this->rand.nextFloat();
			this->CarveCave(chunk, offset, tunnelRadius, carveYaw, carvePitch, 0, 0, 1.0);
		}
	}
}

void CaveGenerator::CarveCave(Chunk& chunk, Vec3 offset) {
	this->CarveCave(chunk, offset, 1.0f + this->rand.nextFloat() * 6.0f,
		0.0f, 0.0f, -1, -1, 0.5);
}

void CaveGenerator::CarveCave(Chunk& chunk, Vec3 offset,
	float tunnelRadius, float carveYaw, float carvePitch,
	int32_t tunnelStep, int32_t tunnelLength,
	double verticalScale) {
	double chunkCenterX = double(chunk.cpos.x * CHUNK_WIDTH + (CHUNK_WIDTH*0.5));
	double chunkCenterZ = double(chunk.cpos.z * CHUNK_WIDTH + (CHUNK_WIDTH*0.5));
	float pitchVel = 0.0f;
	float yawVel = 0.0f;
	Java::Random rand2(this->rand.nextLong());

	if (tunnelLength <= 0) {
		int32_t max_tunnel_length = this->carveExtentLimit * CHUNK_WIDTH - CHUNK_WIDTH;
		tunnelLength = max_tunnel_length - rand2.nextInt(max_tunnel_length / 4);
	}

	bool branch_tunnel = false;
	if (tunnelStep == -1) {
		tunnelStep = tunnelLength / 2;
		branch_tunnel = true;
	}

	int32_t branchPoint = rand2.nextInt(tunnelLength / 2) + tunnelLength / 4;
	
	bool tunnel_steepness = rand2.nextInt(6) == 0;
	for (; tunnelStep < tunnelLength; ++tunnelStep) {
		double radius_xz = 1.5 + double(MathHelper::sin(float(tunnelStep) * JavaMath::PI_FLOAT / float(tunnelLength)) *
			tunnelRadius * 1.0f);
		double radius_y = radius_xz * verticalScale;
		float pCos = MathHelper::cos(carvePitch);
		float pSin = MathHelper::sin(carvePitch);
		offset.x += double(MathHelper::cos(carveYaw) * pCos);
		offset.y += double(pSin);
		offset.z += double(MathHelper::sin(carveYaw) * pCos);

		carvePitch *= tunnel_steepness ? 0.92f : 0.7f;

		carvePitch += yawVel   * 0.1f;
		carveYaw   += pitchVel * 0.1f;
		yawVel   *= 0.9f;
		pitchVel *= 12.0f / 16.0f;
		yawVel 	 += (rand2.nextFloat() - rand2.nextFloat()) * rand2.nextFloat() * 2.0f;
		pitchVel += (rand2.nextFloat() - rand2.nextFloat()) * rand2.nextFloat() * 4.0f;

		if (!branch_tunnel && tunnelStep == branchPoint && tunnelRadius > 1.0f) {
			this->CarveCave(chunk, offset, rand2.nextFloat() * 0.5f + 0.5f,
				carveYaw - JavaMath::PI_FLOAT * 0.5f, carvePitch / 3.0f,
				tunnelStep, tunnelLength, 1.0);
			this->CarveCave(chunk, offset, rand2.nextFloat() * 0.5f + 0.5f,
				carveYaw + JavaMath::PI_FLOAT * 0.5f, carvePitch / 3.0f,
				tunnelStep, tunnelLength, 1.0);
			return;
		}

		if (branch_tunnel || rand2.nextInt(4) != 0) {
			double dx = offset.x - chunkCenterX;
			double dz = offset.z - chunkCenterZ;
			double dist = double(tunnelLength - tunnelStep);
			double limit = double(tunnelRadius + 2.0f + 16.0f);
			if ((dx * dx + dz * dz - dist * dist) > (limit * limit))
				return;

			if (offset.x >= chunkCenterX - 16.0 - radius_xz * 2.0 &&
				offset.z >= chunkCenterZ - 16.0 - radius_xz * 2.0 &&
				offset.x <= chunkCenterX + 16.0 + radius_xz * 2.0 &&
				offset.z <= chunkCenterZ + 16.0 + radius_xz * 2.0) {

				int32_t xMin = MathHelper::floor_double(offset.x - radius_xz) - chunk.cpos.x * 16 - 1;
				int32_t xMax = MathHelper::floor_double(offset.x + radius_xz) - chunk.cpos.x * 16 + 1;
				int32_t yMin = MathHelper::floor_double(offset.y - radius_y) - 1;
				int32_t yMax = MathHelper::floor_double(offset.y + radius_y) + 1;
				int32_t zMin = MathHelper::floor_double(offset.z - radius_xz) - chunk.cpos.z * 16 - 1;
				int32_t zMax = MathHelper::floor_double(offset.z + radius_xz) - chunk.cpos.z * 16 + 1;

				if (xMin < 0)   xMin = 0;
				if (xMax > 16)  xMax = 16;
				if (yMin < 1)   yMin = 1;
				if (yMax > 120) yMax = 120;
				if (zMin < 0)   zMin = 0;
				if (zMax > 16)  zMax = 16;

				// Check for water before carving
				bool waterIsPresent = false;
				for (int32_t blockX = xMin; !waterIsPresent && blockX < xMax; ++blockX) {
					for (int32_t blockZ = zMin; !waterIsPresent && blockZ < zMax; ++blockZ) {
						for (int32_t blockY = yMax + 1; !waterIsPresent && blockY >= yMin - 1; --blockY) {
							if (blockY >= 0 && blockY < CHUNK_HEIGHT) {
								BlockType blockType = chunk.getBlock({ blockX, blockY, blockZ });
								if (blockType == BLOCK_WATER_FLOWING || blockType == BLOCK_WATER_STILL)
									waterIsPresent = true;
								// Skip interior — only check the shell
								if (blockY != yMin - 1 && blockX != xMin && blockX != xMax - 1 &&
									blockZ != zMin && blockZ != zMax - 1) {
									blockY = yMin;
								}
							}
						}
					}
				}

				if (!waterIsPresent) {
					for (int32_t blockX = xMin; blockX < xMax; ++blockX) {
						double center_dx = (double(blockX + chunk.cpos.x * 16) + 0.5 - offset.x) / radius_xz;

						for (int32_t blockZ = zMin; blockZ < zMax; ++blockZ) {
							double center_dz = (double(blockZ + chunk.cpos.z * 16) + 0.5 - offset.z) / radius_xz;

							if (center_dx * center_dx + center_dz * center_dz < 1.0) {
								bool isGrass = false;
								for (int32_t blockY = yMax - 1; blockY >= yMin; --blockY) {
									Int3 bpos{ blockX, blockY + 1, blockZ };
									double center_dy = (double(blockY) + 0.5 - offset.y) / radius_y;
									if (center_dy > -0.7 && center_dx * center_dx + center_dy * center_dy + center_dz * center_dz < 1.0) {
										BlockType blockType = chunk.getBlock(bpos);
										if (blockType == BLOCK_GRASS) isGrass = true;
										if (
											blockType != BLOCK_STONE &&
											blockType != BLOCK_DIRT  &&
											blockType != BLOCK_GRASS
										) {
											continue;
										}
										if (blockY < 10) {
											chunk.setBlock(bpos, BLOCK_LAVA_FLOWING);
											continue;
										}
										chunk.setBlock(bpos, BLOCK_AIR);
										if (!isGrass) continue;
										Int3 below{ bpos.x, blockY, bpos.z };
										if (chunk.getBlock(below) == BLOCK_DIRT) {
											chunk.setBlock(below, BLOCK_GRASS);
										}
									}
								}
							}
						}
					}

					if (branch_tunnel)
						break;
				}
			}
		}
	}
}