/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "cave_gen.h"
#include "java_math.h"

/**
 * @brief Attempts to generate a cave in the current chunk
 *
 * @param chunk The chunk to carve caves into
 * @param seed  The world seed
 */
void CaveGenerator::GenerateCavesForChunk(Chunk& _chunk, int64_t _seed) {
	int32_t carveExtent = this->M_CARVE_EXTENT_LIMIT;
	rand.SetSeed(_seed);
	int64_t xOffset = rand.NextLong() / 2L * 2L + 1L;
	int64_t zOffset = rand.NextLong() / 2L * 2L + 1L;

	// Use unsigned arithmetic to avoid overflow UB
	uint64_t xOffsetU = static_cast<uint64_t>(xOffset);
	uint64_t zOffsetU = static_cast<uint64_t>(zOffset);
	uint64_t seedU = static_cast<uint64_t>(_seed);

	for (int32_t cXoffset = _chunk.cpos.x - carveExtent; cXoffset <= _chunk.cpos.x + carveExtent; ++cXoffset) {
		for (int32_t cZoffset = _chunk.cpos.z - carveExtent; cZoffset <= _chunk.cpos.z + carveExtent; ++cZoffset) {
			uint64_t xPart = static_cast<uint64_t>(static_cast<int64_t>(cXoffset)) * xOffsetU;
			uint64_t zPart = static_cast<uint64_t>(static_cast<int64_t>(cZoffset)) * zOffsetU;
			uint64_t combined = (xPart + zPart) ^ seedU;
			rand.SetSeed(static_cast<int64_t>(combined));
			this->GenerateCaves(_chunk, { cXoffset, cZoffset });
		}
	}
}

void CaveGenerator::GenerateCaves(Chunk& _chunk, Int2 _chunkOffset) {
	int32_t numberOfCaves = rand.NextInt(rand.NextInt(rand.NextInt(isNetherCave ? 10 : 40) + 1) + 1);
	if (rand.NextInt(isNetherCave ? 5 : 15) != 0) {
		numberOfCaves = 0;
	}

	for (int32_t caveIndex = 0; caveIndex < numberOfCaves; ++caveIndex) {
		Vec3 offset = VEC3_ZERO;
		offset.x = double(_chunkOffset.x * CHUNK_WIDTH + rand.NextInt(CHUNK_WIDTH));
		offset.y = isNetherCave ? double(rand.NextInt(128)) : double(rand.NextInt(rand.NextInt(120) + 8));
		offset.z = double(_chunkOffset.y * CHUNK_WIDTH + rand.NextInt(CHUNK_WIDTH));
		int32_t numberOfNodes = 1;
		if (rand.NextInt(4) == 0) {
			this->CarveCave(_chunk, offset);
			numberOfNodes += rand.NextInt(4);
		}

		for (int32_t nodeIndex = 0; nodeIndex < numberOfNodes; ++nodeIndex) {
			float carveYaw = rand.NextFloat() * JavaMath::PI_FLOAT * 2.0f;
			float carvePitch = (rand.NextFloat() - 0.5f) * 2.0f / 8.0f;
			float tunnelRadius = rand.NextFloat() * 2.0f + rand.NextFloat();
			this->CarveCave(_chunk, offset, isNetherCave ? tunnelRadius * 2.0f : tunnelRadius, carveYaw, carvePitch, 0,
			                0, 1.0);
		}
	}
}

void CaveGenerator::CarveCave(Chunk& _chunk, Vec3 _offset) {
	this->CarveCave(_chunk, _offset, 1.0f + rand.NextFloat() * 6.0f, 0.0f, 0.0f, -1, -1, 0.5);
}

void CaveGenerator::CarveCave(Chunk& _chunk, Vec3 _offset, float _tunnelRadius, float _carveYaw, float _carvePitch,
                              int32_t _tunnelStep, int32_t _tunnelLength, double _verticalScale) {
	double chunkCenterX = double(_chunk.cpos.x * CHUNK_WIDTH + (CHUNK_WIDTH * 0.5));
	double chunkCenterZ = double(_chunk.cpos.z * CHUNK_WIDTH + (CHUNK_WIDTH * 0.5));
	float pitchVel = 0.0f;
	float yawVel = 0.0f;
	Java::Random rand2(rand.NextLong());

	if (_tunnelLength <= 0) {
		int32_t maxTunnelLength = this->M_CARVE_EXTENT_LIMIT * CHUNK_WIDTH - CHUNK_WIDTH;
		_tunnelLength = maxTunnelLength - rand2.NextInt(maxTunnelLength / 4);
	}

	bool branchTunnel = false;
	if (_tunnelStep == -1) {
		_tunnelStep = _tunnelLength / 2;
		branchTunnel = true;
	}

	int32_t branchPoint = rand2.NextInt(_tunnelLength / 2) + _tunnelLength / 4;

	bool tunnelSteepness = rand2.NextInt(6) == 0;
	for (; _tunnelStep < _tunnelLength; ++_tunnelStep) {
		double radiusXz = 1.5 + double(MathHelper::Sin(float(_tunnelStep) * JavaMath::PI_FLOAT / float(_tunnelLength)) *
		                                _tunnelRadius * 1.0f);
		double radiusY = radiusXz * _verticalScale;
		float pCos = MathHelper::Cos(_carvePitch);
		float pSin = MathHelper::Sin(_carvePitch);
		_offset.x += double(MathHelper::Cos(_carveYaw) * pCos);
		_offset.y += double(pSin);
		_offset.z += double(MathHelper::Sin(_carveYaw) * pCos);

		_carvePitch *= tunnelSteepness ? 0.92f : 0.7f;

		_carvePitch += yawVel * 0.1f;
		_carveYaw += pitchVel * 0.1f;
		yawVel *= 0.9f;
		pitchVel *= 12.0f / 16.0f;
		yawVel += (rand2.NextFloat() - rand2.NextFloat()) * rand2.NextFloat() * 2.0f;
		pitchVel += (rand2.NextFloat() - rand2.NextFloat()) * rand2.NextFloat() * 4.0f;

		if (!branchTunnel && _tunnelStep == branchPoint && _tunnelRadius > 1.0f) {
			this->CarveCave(_chunk, _offset, rand2.NextFloat() * 0.5f + 0.5f, _carveYaw - JavaMath::PI_FLOAT * 0.5f,
			                _carvePitch / 3.0f, _tunnelStep, _tunnelLength, 1.0);
			this->CarveCave(_chunk, _offset, rand2.NextFloat() * 0.5f + 0.5f, _carveYaw + JavaMath::PI_FLOAT * 0.5f,
			                _carvePitch / 3.0f, _tunnelStep, _tunnelLength, 1.0);
			return;
		}

		if (branchTunnel || rand2.NextInt(4) != 0) {
			double dx = _offset.x - chunkCenterX;
			double dz = _offset.z - chunkCenterZ;
			double dist = double(_tunnelLength - _tunnelStep);
			double limit = double(_tunnelRadius + 2.0f + 16.0f);
			if ((dx * dx + dz * dz - dist * dist) > (limit * limit))
				return;

			if (_offset.x >= chunkCenterX - 16.0 - radiusXz * 2.0 &&
			    _offset.z >= chunkCenterZ - 16.0 - radiusXz * 2.0 &&
			    _offset.x <= chunkCenterX + 16.0 + radiusXz * 2.0 &&
			    _offset.z <= chunkCenterZ + 16.0 + radiusXz * 2.0) {
				int32_t xMin = MathHelper::FloorDouble(_offset.x - radiusXz) - _chunk.cpos.x * 16 - 1;
				int32_t xMax = MathHelper::FloorDouble(_offset.x + radiusXz) - _chunk.cpos.x * 16 + 1;
				int32_t yMin = MathHelper::FloorDouble(_offset.y - radiusY) - 1;
				int32_t yMax = MathHelper::FloorDouble(_offset.y + radiusY) + 1;
				int32_t zMin = MathHelper::FloorDouble(_offset.z - radiusXz) - _chunk.cpos.z * 16 - 1;
				int32_t zMax = MathHelper::FloorDouble(_offset.z + radiusXz) - _chunk.cpos.z * 16 + 1;

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

				// Check for water before carving
				bool fluidIsPresent = false;
				for (int32_t blockX = xMin; !fluidIsPresent && blockX < xMax; ++blockX) {
					for (int32_t blockZ = zMin; !fluidIsPresent && blockZ < zMax; ++blockZ) {
						for (int32_t blockY = yMax + 1; !fluidIsPresent && blockY >= yMin - 1; --blockY) {
							if (blockY >= 0 && blockY < CHUNK_HEIGHT) {
								BlockType blockType = _chunk.GetBlock({ blockX, blockY, blockZ });
								// Overworld caver check
								if (!isNetherCave &&
								    (blockType == BLOCK_WATER_FLOWING || blockType == BLOCK_WATER_STILL))
									fluidIsPresent = true;
								// Nether caver check
								if (isNetherCave && (blockType == BLOCK_LAVA_FLOWING || blockType == BLOCK_LAVA_STILL))
									fluidIsPresent = true;
								// Skip interior, only check the shell
								if (blockY != yMin - 1 && blockX != xMin && blockX != xMax - 1 && blockZ != zMin &&
								    blockZ != zMax - 1) {
									blockY = yMin;
								}
							}
						}
					}
				}

				if (!fluidIsPresent) {
					for (int32_t blockX = xMin; blockX < xMax; ++blockX) {
						double centerDx = (double(blockX + _chunk.cpos.x * 16) + 0.5 - _offset.x) / radiusXz;

						for (int32_t blockZ = zMin; blockZ < zMax; ++blockZ) {
							double centerDz = (double(blockZ + _chunk.cpos.z * 16) + 0.5 - _offset.z) / radiusXz;

							if (centerDx * centerDx + centerDz * centerDz < 1.0) {
								// Doesn't exist in nether caver
								bool isGrass = false;
								for (int32_t blockY = yMax - 1; blockY >= yMin; --blockY) {
									Int3 bpos{ blockX, blockY + 1, blockZ };
									double centerDy = (double(blockY) + 0.5 - _offset.y) / radiusY;
									if (centerDy > -0.7 &&
									    centerDx * centerDx + centerDy * centerDy + centerDz * centerDz < 1.0) {
										BlockType blockType = _chunk.GetBlock(bpos);
										// Nether caver behavior
										// Dirt and grass check is most likely irrelevant,
										// but it still exists in the Vanilla Nether caver
										if (isNetherCave && (blockType == BLOCK_NETHERRACK || blockType == BLOCK_DIRT ||
										                     blockType == BLOCK_GRASS)) {
											_chunk.SetBlock(bpos, BLOCK_AIR);
											continue;
										}
										// Overworld caver behavior
										if (blockType == BLOCK_GRASS)
											isGrass = true;
										if (blockType != BLOCK_STONE && blockType != BLOCK_DIRT &&
										    blockType != BLOCK_GRASS) {
											continue;
										}
										if (blockY < 10) {
											_chunk.SetBlock(bpos, BLOCK_LAVA_FLOWING);
											continue;
										}
										_chunk.SetBlock(bpos, BLOCK_AIR);
										if (!isGrass)
											continue;
										Int3 below{ bpos.x, blockY, bpos.z };
										if (_chunk.GetBlock(below) == BLOCK_DIRT) {
											_chunk.SetBlock(below, BLOCK_GRASS);
										}
									}
								}
							}
						}
					}

					if (branchTunnel)
						break;
				}
			}
		}
	}
}