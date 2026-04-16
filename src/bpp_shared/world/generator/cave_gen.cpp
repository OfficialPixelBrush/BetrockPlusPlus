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
			float carveYaw = this->rand.nextFloat() * float(JavaMath::PI) * 2.0F;
			float carvePitch = (this->rand.nextFloat() - 0.5F) * 2.0F / 8.0F;
			float tunnelRadius = this->rand.nextFloat() * 2.0F + this->rand.nextFloat();
			this->CarveCave(chunk, offset, tunnelRadius, carveYaw, carvePitch, 0, 0, 1.0);
		}
	}
}

void CaveGenerator::CarveCave(Chunk& chunk, Vec3 offset) {
	this->CarveCave(chunk, offset, 1.0F + this->rand.nextFloat() * 6.0F,
		0.0F, 0.0F, -1, -1, 0.5);
}

void CaveGenerator::CarveCave(Chunk& chunk, Vec3 offset,
	float tunnelRadius, float carveYaw, float carvePitch,
	int32_t tunnelStep, int32_t tunnelLength,
	double verticalScale) {
	double chunkCenterX = double(chunk.cpos.x * CHUNK_WIDTH + 8);
	double chunkCenterZ = double(chunk.cpos.z * CHUNK_WIDTH + 8);
	float var21 = 0.0F;
	float var22 = 0.0F;
	Java::Random rand2 = Java::Random(this->rand.nextLong());

	if (tunnelLength <= 0) {
		int32_t var24 = this->carveExtentLimit * 16 - 16;
		tunnelLength = var24 - rand2.nextInt(var24 / 4);
	}

	bool var52 = false;
	if (tunnelStep == -1) {
		tunnelStep = tunnelLength / 2;
		var52 = true;
	}

	int32_t var25 = rand2.nextInt(tunnelLength / 2) + tunnelLength / 4;

	for (bool var26 = rand2.nextInt(6) == 0; tunnelStep < tunnelLength; ++tunnelStep) {
		double var27 = 1.5 + double(MathHelper::sin(float(tunnelStep) * float(JavaMath::PI) / float(tunnelLength)) *
			tunnelRadius * 1.0F);
		double var29 = var27 * verticalScale;
		float var31 = MathHelper::cos(carvePitch);
		float var32 = MathHelper::sin(carvePitch);
		offset.x += double(MathHelper::cos(carveYaw) * var31);
		offset.y += double(var32);
		offset.z += double(MathHelper::sin(carveYaw) * var31);

		if (var26) {
			carvePitch *= 0.92F;
		}
		else {
			carvePitch *= 0.7F;
		}

		carvePitch += var22 * 0.1F;
		carveYaw += var21 * 0.1F;
		var22 *= 0.9F;
		var21 *= 12.0F / 16.0F;
		var22 += (rand2.nextFloat() - rand2.nextFloat()) * rand2.nextFloat() * 2.0F;
		var21 += (rand2.nextFloat() - rand2.nextFloat()) * rand2.nextFloat() * 4.0F;

		if (!var52 && tunnelStep == var25 && tunnelRadius > 1.0F) {
			this->CarveCave(chunk, offset, rand2.nextFloat() * 0.5F + 0.5F,
				carveYaw - float(JavaMath::PI) * 0.5F, carvePitch / 3.0F,
				tunnelStep, tunnelLength, 1.0);
			this->CarveCave(chunk, offset, rand2.nextFloat() * 0.5F + 0.5F,
				carveYaw + float(JavaMath::PI) * 0.5F, carvePitch / 3.0F,
				tunnelStep, tunnelLength, 1.0);
			return;
		}

		if (var52 || rand2.nextInt(4) != 0) {
			double var33 = offset.x - chunkCenterX;
			double var35 = offset.z - chunkCenterZ;
			double var37 = double(tunnelLength - tunnelStep);
			double var39 = double(tunnelRadius + 2.0F + 16.0F);
			if (var33 * var33 + var35 * var35 - var37 * var37 > var39 * var39) {
				return;
			}

			if (offset.x >= chunkCenterX - 16.0 - var27 * 2.0 &&
				offset.z >= chunkCenterZ - 16.0 - var27 * 2.0 &&
				offset.x <= chunkCenterX + 16.0 + var27 * 2.0 &&
				offset.z <= chunkCenterZ + 16.0 + var27 * 2.0) {

				int32_t xMin = MathHelper::floor_double(offset.x - var27) - chunk.cpos.x * 16 - 1;
				int32_t xMax = MathHelper::floor_double(offset.x + var27) - chunk.cpos.x * 16 + 1;
				int32_t yMin = MathHelper::floor_double(offset.y - var29) - 1;
				int32_t yMax = MathHelper::floor_double(offset.y + var29) + 1;
				int32_t zMin = MathHelper::floor_double(offset.z - var27) - chunk.cpos.z * 16 - 1;
				int32_t zMax = MathHelper::floor_double(offset.z + var27) - chunk.cpos.z * 16 + 1;

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
								if (blockType == BLOCK_WATER_FLOWING || blockType == BLOCK_WATER_STILL) {
									waterIsPresent = true;
								}
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
						double var57 = (double(blockX + chunk.cpos.x * 16) + 0.5 - offset.x) / var27;

						for (int32_t blockZ = zMin; blockZ < zMax; ++blockZ) {
							double var44 = (double(blockZ + chunk.cpos.z * 16) + 0.5 - offset.z) / var27;

							if (var57 * var57 + var44 * var44 < 1.0) {
								bool var47 = false;
								// Step down through the y column
								Int3 bpos{ blockX, yMax - 1, blockZ };
								for (; bpos.y >= yMin; --bpos.y) {
									double var49 = (double(bpos.y) + 0.5 - offset.y) / var29;
									if (var49 > -0.7 && var57 * var57 + var49 * var49 + var44 * var44 < 1.0) {
										BlockType blockType = chunk.getBlock(bpos);
										if (blockType == BLOCK_GRASS) {
											var47 = true;
										}
										if (blockType == BLOCK_STONE || blockType == BLOCK_DIRT || blockType == BLOCK_GRASS) {
											if (bpos.y < 10) {
												chunk.setBlock(bpos, BLOCK_LAVA_FLOWING);
											}
											else {
												chunk.setBlock(bpos, BLOCK_AIR);
												if (var47) {
													Int3 below{ bpos.x, bpos.y - 1, bpos.z };
													if (chunk.getBlock(below) == BLOCK_DIRT) {
														chunk.setBlock(below, BLOCK_GRASS);
													}
												}
											}
										}
									}
								}
							}
						}
					}

					if (var52) {
						break;
					}
				}
			}
		}
	}
}