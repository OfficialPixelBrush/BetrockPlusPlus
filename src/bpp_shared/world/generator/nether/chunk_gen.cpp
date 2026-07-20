/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "chunk_gen.h"
#include "chunk.h"

/**
 * @brief Construct a new Beta 1.7.3 Nether Generator
 *
 * @param pSeed The seed of the generated world
 * @param pWorld The world that the NetherGenerator belongs to
 */
NetherGenerator::NetherGenerator(int64_t p_seed) : Generator(p_seed), caver(true) {
	// Tell caver it's a nether caver
	rand = Java::Random(seed);
	// Init Terrain Noise
	lowNoiseGen = NoiseOctavesPerlin(rand, 16);
	highNoiseGen = NoiseOctavesPerlin(rand, 16);
	selectorNoiseGen = NoiseOctavesPerlin(rand, 8);
	sandGravelNoiseGen = NoiseOctavesPerlin(rand, 4);
	stoneNoiseGen = NoiseOctavesPerlin(rand, 4);
	continentalnessNoiseGen = NoiseOctavesPerlin(rand, 10);
	depthNoiseGen = NoiseOctavesPerlin(rand, 16);
}

/**
 * @brief Generate a non-populated chunk
 *
 * @param chunkPos The x,z coordinate of the chunk
 * @return std::shared_ptr<Chunk>
 */
void NetherGenerator::GenerateChunk(Chunk& chunk) {
	rand.setSeed(int64_t(chunk.cpos.x) * 341873128712L + int64_t(chunk.cpos.z) * 132897987541L);

	// Allocate empty chunk
	chunk.clear();
	// Generate the Terrain, minus any caves, as just stone
	GenerateTerrain(chunk);
	// Replace some of the stone with Biome-appropriate blocks
	ReplaceBlocksForBiome(chunk);
	// Carve caves
	caver.GenerateCavesForChunk(chunk, seed);
	// Generate heightmap
	chunk.generateHeightMap();

	chunk.isModified = true;
}

/**
 * @brief Replace some of the stone with Biome-appropriate blocks
 *
 * @param chunkPos The x,z coordinate of the chunk
 * @param c The chunk that should gets its blocks replaced
 */
void NetherGenerator::ReplaceBlocksForBiome(Chunk& chunk) {
	const double oneThirtySecond = 1.0 / 32.0;
	// Init noise maps
	sandNoise.resize(256, 0.0);
	gravelNoise.resize(256, 0.0);
	stoneNoise.resize(256, 0.0);

	// Populate noise maps
	sandGravelNoiseGen.GenerateOctaves(
	    sandNoise, Vec3{ double(chunk.cpos.x * CHUNK_WIDTH), double(chunk.cpos.z * CHUNK_WIDTH), 0.0 },
	    Int32_3{ 16, 16, 1 }, Vec3{ oneThirtySecond, oneThirtySecond, 1.0 });
	sandGravelNoiseGen.GenerateOctaves(
	    gravelNoise, Vec3{ double(chunk.cpos.x * CHUNK_WIDTH), 109.0134, double(chunk.cpos.z * CHUNK_WIDTH) },
	    Int32_3{ 16, 1, 16 }, Vec3{ oneThirtySecond, 1.0, oneThirtySecond });
	stoneNoiseGen.GenerateOctaves(stoneNoise,
	                                Vec3{ double(chunk.cpos.x * CHUNK_WIDTH), double(chunk.cpos.z * CHUNK_WIDTH), 0.0 },
	                                Int32_3{ 16, 16, 1 },
	                                Vec3{ oneThirtySecond * 2.0, oneThirtySecond * 2.0, oneThirtySecond * 2.0 });

	// Iterate through entire chunk
	for (int32_t x = 0; x < CHUNK_WIDTH; ++x) {
		for (int32_t z = 0; z < CHUNK_WIDTH; ++z) {
			// This is intentional, to match b1.7.3 behavior!
			size_t bindex = size_t(x + z * CHUNK_WIDTH);
			// Get values from noise maps
			bool sandActive = sandNoise[bindex] + rand.nextDouble() * 0.2 > 0.0;
			bool gravelActive = gravelNoise[bindex] + rand.nextDouble() * 0.2 > 3.0;
			int32_t stoneActive = Java::DoubleToInt32(stoneNoise[bindex] / 3.0 + 3.0 + rand.nextDouble() * 0.25);
			int32_t stoneDepth = -1;
			// Get biome-appropriate top and filler blocks
			BlockType topBlock = BLOCK_NETHERRACK;
			BlockType fillerBlock = BLOCK_NETHERRACK;

			// Iterate over column top to bottom
			for (int32_t y = CHUNK_HEIGHT - 1; y >= 0; --y) {
				// This is intentional, to match b1.7.3 behavior!
				Int3 bpos{ z, y, x };
				// Place Bedrock at bottom and top with some randomness
				if (y >= (CHUNK_HEIGHT - 1) - rand.nextInt(5)) {
					chunk.setBlock(bpos, BLOCK_BEDROCK);
					continue;
				} else if (y <= 0 + rand.nextInt(5)) {
					chunk.setBlock(bpos, BLOCK_BEDROCK);
					continue;
				}

				BlockType currentBlock = chunk.getBlock(bpos);
				// Ignore air
				if (currentBlock == BLOCK_AIR) {
					stoneDepth = -1;
					continue;
				}

				// If we counter stone, start replacing it
				if (currentBlock == BLOCK_NETHERRACK) {
					if (stoneDepth == -1) {
						if (stoneActive <= 0) {
							topBlock = BLOCK_AIR;
							fillerBlock = BLOCK_NETHERRACK;
						} else if (y >= NETHER_BIOME_LAVA_LEVEL - 4 && y <= NETHER_BIOME_LAVA_LEVEL + 1) {
							// If we're close to the water level, apply gravel and sand
							topBlock = BLOCK_NETHERRACK;
							fillerBlock = BLOCK_NETHERRACK;

							if (gravelActive) {
								topBlock = BLOCK_GRAVEL;
								fillerBlock = BLOCK_NETHERRACK;
							}
							if (sandActive) {
								topBlock = BLOCK_SOULSAND;
								fillerBlock = BLOCK_SOULSAND;
							}
						}

						// Add water if we're below lava level
						if (y < NETHER_BIOME_LAVA_LEVEL && topBlock == BLOCK_AIR) {
							topBlock = BLOCK_LAVA_STILL;
						}

						stoneDepth = stoneActive;
						// Place filler block if we're under lava
						chunk.setBlock(bpos, (y >= NETHER_BIOME_LAVA_LEVEL - 1) ? topBlock : fillerBlock);
					} else if (stoneDepth > 0) {
						--stoneDepth;
						chunk.setBlock(bpos, fillerBlock);
					}
				}
			}
		}
	}
}

/**
 * @brief Generate the Terrain, minus any caves, as just stone
 *
 * @param chunkPos The x,z coordinate of the chunk
 * @param c The chunk that should get its terrain generated
 */
void NetherGenerator::GenerateTerrain(Chunk& chunk) {
	const Int3 max{ CHUNK_WIDTH / 4 + 1, CHUNK_HEIGHT / 8 + 1, CHUNK_WIDTH / 4 + 1 };

	// Generate 4x16x4 low resolution noise map
	GenerateTerrainNoise(Int3{ chunk.cpos.x * 4, 0, chunk.cpos.z * 4 }, max);

	// Terrain noise is interpolated and only sampled every 4 blocks
	for (int32_t sampleX = 0; sampleX < 4; ++sampleX) {
		for (int32_t sampleZ = 0; sampleZ < 4; ++sampleZ) {
			for (int32_t sampleY = 0; sampleY < 16; ++sampleY) {
				double verticalLerpStep = 0.125;

				// Get noise cube corners
				double corner000 =
				    terrainNoiseField[size_t(((sampleX + 0) * max.z + sampleZ + 0) * max.y + sampleY + 0)];
				double corner010 =
				    terrainNoiseField[size_t(((sampleX + 0) * max.z + sampleZ + 1) * max.y + sampleY + 0)];
				double corner100 =
				    terrainNoiseField[size_t(((sampleX + 1) * max.z + sampleZ + 0) * max.y + sampleY + 0)];
				double corner110 =
				    terrainNoiseField[size_t(((sampleX + 1) * max.z + sampleZ + 1) * max.y + sampleY + 0)];
				double corner001 =
				    (terrainNoiseField[size_t(((sampleX + 0) * max.z + sampleZ + 0) * max.y + sampleY + 1)] -
				     corner000) *
				    verticalLerpStep;
				double corner011 =
				    (terrainNoiseField[size_t(((sampleX + 0) * max.z + sampleZ + 1) * max.y + sampleY + 1)] -
				     corner010) *
				    verticalLerpStep;
				double corner101 =
				    (terrainNoiseField[size_t(((sampleX + 1) * max.z + sampleZ + 0) * max.y + sampleY + 1)] -
				     corner100) *
				    verticalLerpStep;
				double corner111 =
				    (terrainNoiseField[size_t(((sampleX + 1) * max.z + sampleZ + 1) * max.y + sampleY + 1)] -
				     corner110) *
				    verticalLerpStep;

				// Interpolate the 1/4th scale noise
				for (int32_t subY = 0; subY < 8; ++subY) {
					double horizontalLerpStep = 0.25;
					double terrainX0 = corner000;
					double terrainX1 = corner010;
					double terrainStepX0 = (corner100 - corner000) * horizontalLerpStep;
					double terrainStepX1 = (corner110 - corner010) * horizontalLerpStep;

					for (int32_t subX = 0; subX < 4; ++subX) {
						Int3 bpos{ (subX + sampleX * 4), ((sampleY * 8) + subY), (sampleZ * 4) };
						double terrainDensity = terrainX0;
						double densityStepZ = (terrainX1 - terrainX0) * horizontalLerpStep;

						for (int32_t subZ = 0; subZ < 4; ++subZ) {
							// Here the actual block is determined
							// Default to air block
							BlockType blockType = BLOCK_AIR;

							// Place lava in empty space below Nether lava level
							int32_t yLevel = sampleY * 8 + subY;
							if (yLevel < NETHER_LAVA_LEVEL)
								blockType = BLOCK_LAVA_STILL;

							// If the terrain density falls below,
							// replace block with stone
							if (terrainDensity > 0.0)
								blockType = BLOCK_NETHERRACK;

							chunk.setBlock(bpos, blockType);
							// Prep for next iteration
							bpos.z += 1;
							terrainDensity += densityStepZ;
						}

						terrainX0 += terrainStepX0;
						terrainX1 += terrainStepX1;
					}

					corner000 += corner001;
					corner010 += corner011;
					corner100 += corner101;
					corner110 += corner111;
				}
			}
		}
	}
}

/**
 * @brief Make terrain noise and updates the terrain map
 *
 * @param terrainMap The terrain map that the scaled-down terrain values will be written to
 * @param chunkPos The x,y,z coordinate of the sub-chunk
 * @param max Defines the area of the terrainMap
 */
void NetherGenerator::GenerateTerrainNoise(Int3 cpos, Int3 max) {
	double horiScale = 684.412;
	double vertScale = 2053.236;

	{
		Vec3 vecCpos = Vec3{ double(cpos.x), double(cpos.y), double(cpos.z) };
		// We do this to need to generate noise as often
		continentalnessNoiseGen.GenerateOctaves(continentalnessNoiseField, vecCpos, Int32_3{ max.x, 1, max.z },
		                                          Vec3{ 1.0, 0.0, 1.0 });
		depthNoiseGen.GenerateOctaves(depthNoiseField, vecCpos, Int32_3{ max.x, 1, max.z },
		                                Vec3{ 100.0, 0.0, 100.0 });
		selectorNoiseGen.GenerateOctaves(selectorNoiseField, vecCpos, max,
		                                   Vec3{ horiScale / 80.0, vertScale / 60.0, horiScale / 80.0 });
		lowNoiseGen.GenerateOctaves(lowNoiseField, vecCpos, max, Vec3{ horiScale, vertScale, horiScale });
		highNoiseGen.GenerateOctaves(highNoiseField, vecCpos, max, Vec3{ horiScale, vertScale, horiScale });
	}
	// Used to iterate 3D noise maps (low, high, selector)
	size_t xyzIndex = 0;
	// Used to iterate 2D Noise maps (depth, continentalness)
	size_t xzIndex = 0;
	// Reserve stuff
	std::vector<double> netherDensityOffset(size_t(max.y));

	for (int iY = 0; iY < max.y; ++iY) {
		netherDensityOffset[size_t(iY)] = std::cos(double(iY) * JavaMath::PI * 6.0 / double(max.y)) * 2.0;
		double diY = double(iY);
		if (diY > max.y / 2)
			diY = double(max.y - 1 - iY);
		if (diY < 4.0) {
			diY = 4.0 - diY;
			netherDensityOffset[size_t(iY)] -= diY * diY * diY * 10.0;
		}
	}

	for (int32_t iX = 0; iX < max.x; ++iX) {
		for (int32_t iZ = 0; iZ < max.z; ++iZ) {
			// Sample contientalness noise
			double continentalness = (continentalnessNoiseField[xzIndex] + 256.0) / 512.0;
			if (continentalness > 1.0)
				continentalness = 1.0;
			// Sample depth noise
			double depthNoise = depthNoiseField[xzIndex] / 8000.0;
			if (depthNoise < 0.0)
				depthNoise = -depthNoise;
			depthNoise = depthNoise * 3.0 - 3.0;
			if (depthNoise < 0.0) {
				depthNoise /= 2.0;
				if (depthNoise < -1.0)
					depthNoise = -1.0;
				depthNoise /= 1.4;
				depthNoise /= 2.0;
				continentalness = 0.0;
			} else {
				if (depthNoise > 1.0)
					depthNoise = 1.0;
				depthNoise /= 6.0;
			}
			continentalness += 0.5;
			depthNoise = depthNoise * double(max.y) / 16.0;
			++xzIndex;

			for (int32_t iY = 0; iY < max.y; ++iY) {
				// Sample 3D noises
				double terrainDensity = 0.0;
				double densityOffset = netherDensityOffset[iY];
				// Sample low noise
				double lowNoise = lowNoiseField[xyzIndex] / 512.0;
				// Sample high noise
				double highNoise = highNoiseField[xyzIndex] / 512.0;
				// Sample selector noise
				double selectorNoise = (selectorNoiseField[xyzIndex] / 10.0 + 1.0) / 2.0;
				if (selectorNoise < 0.0) {
					terrainDensity = lowNoise;
				} else if (selectorNoise > 1.0) {
					terrainDensity = highNoise;
				} else {
					terrainDensity = lowNoise + (highNoise - lowNoise) * selectorNoise;
				}

				terrainDensity -= densityOffset;
				// Reduce density towards max height
				if (iY > max.y - 4) {
					double heightEdgeFade = double(float(iY - (max.y - 4)) / 3.0F);
					terrainDensity = (terrainDensity * (1.0 - heightEdgeFade)) + (-10.0 * heightEdgeFade);
				}
				if (double(iY) < 0.0) {
					double heightEdgeFade = (0.0 - double(iY) / 4.0);
					if (heightEdgeFade < 0.0)
						heightEdgeFade = 0.0;
					if (heightEdgeFade > 1.0)
						heightEdgeFade = 1.0;
					terrainDensity = (terrainDensity * (1.0 - heightEdgeFade)) + (-10.0 * heightEdgeFade);
				}

				terrainNoiseField[xyzIndex] = terrainDensity;
				++xyzIndex;
			}
		}
	}
}

/**
 * @brief Populates the specified chunk with biome-specific features.
 *
 * Direct port of ChunkProviderHell.populate() from Beta 1.7.3.
 * Section order, rand call counts, and coordinate offsets all
 * match the Java source exactly.
 */
bool NetherGenerator::PopulateChunk(Chunk& chunk, WorldWrapper& world) {
	const int32_t blockX = chunk.cpos.x * CHUNK_WIDTH;
	const int32_t blockZ = chunk.cpos.z * CHUNK_WIDTH;
	// TODO: The nether does not initialize its prng values,
	// meaning that *technically* they're fully up to random chance.
	// It just happens that this random chance is somewhat consistent, apparently.
	// So... figure that out, if possible. Probably just some chunk ordering tomfuckery.
	rand.setSeed(world.getSeed());
	int64_t xSalt = rand.nextLong() / 2L * 2L + 1L;
	int64_t zSalt = rand.nextLong() / 2L * 2L + 1L;
	// Use unsigned arithmetic to avoid overflow UB
	uint64_t xSalt_u = static_cast<uint64_t>(xSalt);
	uint64_t zSalt_u = static_cast<uint64_t>(zSalt);
	uint64_t xPart = static_cast<uint64_t>(static_cast<int64_t>(chunk.cpos.x)) * xSalt_u;
	uint64_t zPart = static_cast<uint64_t>(static_cast<int64_t>(chunk.cpos.z)) * zSalt_u;
	uint64_t combined = (xPart + zPart) ^ static_cast<uint64_t>(world.getSeed());

	rand.setSeed(static_cast<int64_t>(combined));

	Int3 coord;
	// Generate single-block lava streams
	for (int attempt = 0; attempt < 8; ++attempt) {
		coord.x = blockX + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		coord.y = rand.nextInt(CHUNK_HEIGHT - 8) + 4; // 120
		coord.z = blockZ + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		FeatureGenerator(BLOCK_LAVA_FLOWING).GenerateNetherLiquid(world, rand, coord);
	}

	// Generate fire patch
	int max_fire_attempts = rand.nextInt(rand.nextInt(10) + 1) + 1;
	for (int attempt = 0; attempt < max_fire_attempts; ++attempt) {
		coord.x = blockX + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		coord.y = rand.nextInt(CHUNK_HEIGHT - 8) + 4; // 120
		coord.z = blockZ + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		FeatureGenerator().GenerateNetherFire(world, rand, coord);
	}

	// Generate Glowstone Blob
	int max_glowstone_attempts = rand.nextInt(rand.nextInt(10) + 1);
	for (int attempt = 0; attempt < max_glowstone_attempts; ++attempt) {
		coord.x = blockX + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		coord.y = rand.nextInt(CHUNK_HEIGHT - 8) + 4; // 120
		coord.z = blockZ + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		FeatureGenerator().GenerateNetherGlowstone(world, rand, coord);
	}

	// Generate secondary Glowstone Blob
	for (int attempt = 0; attempt < 10; ++attempt) {
		coord.x = blockX + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		coord.y = rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		FeatureGenerator().GenerateNetherGlowstone(world, rand, coord);
	}

	// Generate Brown Mushrooms
	if (rand.nextInt(1) == 0) {
		coord.x = blockX + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		coord.y = rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		FeatureGenerator(BLOCK_MUSHROOM_BROWN).GenerateFlowers(world, rand, coord);
	}

	// Generate Red Mushrooms
	if (rand.nextInt(1) == 0) {
		coord.x = blockX + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		coord.y = rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + rand.nextInt(CHUNK_WIDTH) + (CHUNK_WIDTH * 0.5);
		FeatureGenerator(BLOCK_MUSHROOM_RED).GenerateFlowers(world, rand, coord);
	}

	return true;
}