/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "cave_gen.h"
CaveGenerator::CaveGenerator() {}

/**
 * @brief Attempts to generate a cave in the current chunk
 * 
 * @param world The world this cave is being generated in
 * @param chunkPos The (x,z) position of the chunk
 * @param c The pointer to the chunk
 */
void CaveGenerator::GenerateCavesForChunk(World *world, Int2 chunkPos, std::shared_ptr<Chunk> &c) {
	int32_t carveExtent = this->carveExtentLimit;
	this->rand.setSeed(world->seed);
	int64_t xOffset = this->rand.nextLong() / 2L * 2L + 1L;
	int64_t zOffset = this->rand.nextLong() / 2L * 2L + 1L;

	// Iterate beyond the current chunk by 8 chunks in every direction
	for (int32_t cXoffset = chunkPos.x - carveExtent; cXoffset <= chunkPos.x + carveExtent; ++cXoffset) {
		for (int32_t cZoffset = chunkPos.y - carveExtent; cZoffset <= chunkPos.y + carveExtent; ++cZoffset) {
			this->rand.setSeed(((int64_t(cXoffset) * xOffset) + (int64_t(cZoffset) * zOffset)) ^ world->seed);
			this->GenerateCaves(Int2{cXoffset, cZoffset}, chunkPos, c);
		}
	}
}

// TODO: This is only the cave generator for the overworld.
// The one for the nether is different!
void CaveGenerator::GenerateCaves(Int2 chunkOffset, Int2 chunkPos, std::shared_ptr<Chunk> &c) {
	int32_t numberOfCaves = this->rand.nextInt(this->rand.nextInt(this->rand.nextInt(40) + 1) + 1);
	if (this->rand.nextInt(15) != 0) {
		numberOfCaves = 0;
	}

	for (int32_t caveIndex = 0; caveIndex < numberOfCaves; ++caveIndex) {
		Vec3 offset = VEC3_ZERO;
		offset.x = double(chunkOffset.x * CHUNK_WIDTH_X + this->rand.nextInt(CHUNK_WIDTH_X));
		offset.y = double(this->rand.nextInt(this->rand.nextInt(120) + 8));
		offset.z = double(chunkOffset.y * CHUNK_WIDTH_Z + this->rand.nextInt(CHUNK_WIDTH_Z));
		int32_t numberOfNodes = 1;
		if (this->rand.nextInt(4) == 0) {
			this->CarveCave(chunkPos, c, offset);
			numberOfNodes += this->rand.nextInt(4);
		}

		for (int32_t nodeIndex = 0; nodeIndex < numberOfNodes; ++nodeIndex) {
			float carveYaw = this->rand.nextFloat() * float(JavaMath::PI) * 2.0F;
			float carvePitch = (this->rand.nextFloat() - 0.5F) * 2.0F / 8.0F;
			float tunnelRadius = this->rand.nextFloat() * 2.0F + this->rand.nextFloat();
			this->CarveCave(chunkPos, c, offset, tunnelRadius, carveYaw, carvePitch, 0, 0, 1.0);
		}
	}
}

void CaveGenerator::CarveCave(Int2 chunkPos, std::shared_ptr<Chunk> &c, Vec3 offset) {
	this->CarveCave(chunkPos, c, offset, 1.0F + this->rand.nextFloat() * 6.0F, 0.0F, 0.0F, -1, -1,
					0.5);
}

void CaveGenerator::CarveCave(Int2 chunkPos, std::shared_ptr<Chunk> &c, Vec3 offset,
							 float tunnelRadius, float carveYaw, float carvePitch, int32_t tunnelStep, int32_t tunnelLength,
							 double verticalScale) {
	double chunkCenterX = double(chunkPos.x * CHUNK_WIDTH_X + 8);
	double chunkCenterZ = double(chunkPos.y * CHUNK_WIDTH_Z + 8);
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
		} else {
			carvePitch *= 0.7F;
		}

		carvePitch += var22 * 0.1F;
		carveYaw += var21 * 0.1F;
		var22 *= 0.9F;
		var21 *= 12.0F / 16.0F;
		var22 += (rand2.nextFloat() - rand2.nextFloat()) * rand2.nextFloat() * 2.0F;
		var21 += (rand2.nextFloat() - rand2.nextFloat()) * rand2.nextFloat() * 4.0F;
		if (!var52 && tunnelStep == var25 && tunnelRadius > 1.0F) {
			this->CarveCave(chunkPos, c, offset, rand2.nextFloat() * 0.5F + 0.5F,
							carveYaw - (float)JavaMath::PI * 0.5F, carvePitch / 3.0F, tunnelStep, tunnelLength, 1.0);
			this->CarveCave(chunkPos, c, offset, rand2.nextFloat() * 0.5F + 0.5F,
							carveYaw + (float)JavaMath::PI * 0.5F, carvePitch / 3.0F, tunnelStep, tunnelLength, 1.0);
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

			if (offset.x >= chunkCenterX - 16.0 - var27 * 2.0 && offset.z >= chunkCenterZ - 16.0 - var27 * 2.0 &&
				offset.x <= chunkCenterX + 16.0 + var27 * 2.0 && offset.z <= chunkCenterZ + 16.0 + var27 * 2.0) {
				int32_t xMin = MathHelper::floor_double(offset.x - var27) - chunkPos.x * 16 - 1;
				int32_t xMax = MathHelper::floor_double(offset.x + var27) - chunkPos.x * 16 + 1;
				int32_t yMin = MathHelper::floor_double(offset.y - var29) - 1;
				int32_t yMax = MathHelper::floor_double(offset.y + var29) + 1;
				int32_t zMin = MathHelper::floor_double(offset.z - var27) - chunkPos.y * 16 - 1;
				int32_t zMax = MathHelper::floor_double(offset.z + var27) - chunkPos.y * 16 + 1;
				// Limiting to chunk boundaries
				if (xMin < 0)
					xMin = 0;
				if (xMax > 16)
					xMax = 16;
				if (yMin < 1)
					yMin = 1;
				if (yMax > 120)
					yMax = 120;
				if (zMin < 0)
					zMin = 0;
				if (zMax > 16)
					zMax = 16;

				bool waterIsPresent = false;

				for (int32_t blockX = xMin; !waterIsPresent && blockX < xMax; ++blockX) {
					for (int32_t blockZ = zMin; !waterIsPresent && blockZ < zMax; ++blockZ) {
						for (int32_t blockY = yMax + 1; !waterIsPresent && blockY >= yMin - 1; --blockY) {
							int32_t blockIndex = (blockX * CHUNK_WIDTH_Z + blockZ) * CHUNK_HEIGHT + blockY;
							if (blockY >= 0 && blockY < CHUNK_HEIGHT) {

								if (c->GetBlockType(BlockIndexToPosition(blockIndex)) == BLOCK_WATER_FLOWING ||
									c->GetBlockType(BlockIndexToPosition(blockIndex)) == BLOCK_WATER_STILL) {
									waterIsPresent = true;
								}

								if (blockY != yMin - 1 && blockX != xMin && blockX != xMax - 1 && blockZ != zMin &&
									blockZ != zMax - 1) {
									blockY = yMin;
								}
							}
						}
					}
				}

				if (!waterIsPresent) {
					for (int32_t blockX = xMin; blockX < xMax; ++blockX) {
						double var57 = (double(blockX + chunkPos.x * 16) + 0.5 - offset.x) / var27;

						for (int32_t blockZ = zMin; blockZ < zMax; ++blockZ) {
							double var44 = (double(blockZ + chunkPos.y * 16) + 0.5 - offset.z) / var27;
							int32_t blockIndex = (blockX * CHUNK_WIDTH_Z + blockZ) * CHUNK_HEIGHT + yMax;
							bool var47 = false;
							if (var57 * var57 + var44 * var44 < 1.0) {
								for (int32_t var48 = yMax - 1; var48 >= yMin; --var48) {
									double var49 = (double(var48) + 0.5 - offset.y) / var29;
									if (var49 > -0.7 && var57 * var57 + var49 * var49 + var44 * var44 < 1.0) {
										BlockType blockType = c->GetBlockType(BlockIndexToPosition(blockIndex));
										if (blockType == BLOCK_GRASS) {
											var47 = true;
										}

										if (blockType == BLOCK_STONE || blockType == BLOCK_DIRT ||
											blockType == BLOCK_GRASS) {
											if (var48 < 10) {
												c->SetBlockType(BLOCK_LAVA_FLOWING, BlockIndexToPosition(blockIndex));
											} else {
												c->SetBlockType(BLOCK_AIR, BlockIndexToPosition(blockIndex));
												if (var47 && c->GetBlockType(BlockIndexToPosition(blockIndex - 1)) ==
																 BLOCK_DIRT) {
													c->SetBlockType(BLOCK_GRASS, BlockIndexToPosition(blockIndex - 1));
												}
											}
										}
									}

									--blockIndex;
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