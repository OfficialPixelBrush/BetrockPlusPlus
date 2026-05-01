/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "chunk_gen.h"
#include "chunk.h"

/**
 * @brief Construct a new Beta 1.7.3 Generator
 *
 * @param pSeed The seed of the generated world
 * @param pWorld The world that the generator belongs to
 */
Generator::Generator(int64_t p_seed) {
	this->seed = p_seed;
	rand = Java::Random(this->seed);
	// Init Terrain Noise
	lowNoiseGen = NoiseOctavesPerlin(rand, 16);
	highNoiseGen = NoiseOctavesPerlin(rand, 16);
	selectorNoiseGen = NoiseOctavesPerlin(rand, 8);
	sandGravelNoiseGen = NoiseOctavesPerlin(rand, 4);
	stoneNoiseGen = NoiseOctavesPerlin(rand, 4);
	continentalnessNoiseGen = NoiseOctavesPerlin(rand, 10);
	depthNoiseGen = NoiseOctavesPerlin(rand, 16);
	treeDensityNoiseGen = NoiseOctavesPerlin(rand, 8);
}

/**
 * @brief Generate a non-populated chunk
 *
 * @param chunkPos The x,z coordinate of the chunk
 * @return std::shared_ptr<Chunk>
 */
void Generator::GenerateChunk(Chunk& chunk) {
	this->rand.setSeed(int64_t(chunk.cpos.x) * 341873128712L + int64_t(chunk.cpos.z) * 132897987541L);

	// Allocate empty chunk
	chunk.clear();

	// Generate Biomes
	BiomeGenerator(seed).GenerateBiomeMap(biomeMap,
		temperature, humidity, weirdness,
		Int2{ chunk.cpos.x * CHUNK_WIDTH, chunk.cpos.z * CHUNK_WIDTH }
	);

	// Store the final temperature and humidity in the chunk so PopulateChunk
	// (which runs on a different thread_local Generator) can reconstruct the
	// biome map via GetBiomeFromLookup without re-running the noise generators.
	for (int i = 0; i < CHUNK_AREA; ++i) {
		chunk.temperature[i] = float(temperature[i]);
		chunk.humidity[i] = float(humidity[i]);
	}

	// Generate the Terrain, minus any caves, as just stone
	GenerateTerrain(chunk);
	// Replace some of the stone with Biome-appropriate blocks
	ReplaceBlocksForBiome(chunk);
	// Carve caves
	this->caver.GenerateCavesForChunk(chunk, this->seed);
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
void Generator::ReplaceBlocksForBiome(Chunk& chunk) {
	const double oneThirtySecond = 1.0 / 32.0;
	// Init noise maps
	this->sandNoise.resize(256, 0.0);
	this->gravelNoise.resize(256, 0.0);
	this->stoneNoise.resize(256, 0.0);

	// Populate noise maps
	this->sandGravelNoiseGen.GenerateOctaves(this->sandNoise,
		Vec3{double(chunk.cpos.x* CHUNK_WIDTH), double(chunk.cpos.z* CHUNK_WIDTH), 0.0},
		Int32_3{ 16, 16, 1 },
		Vec3{ oneThirtySecond, oneThirtySecond, 1.0 }
	);
	this->sandGravelNoiseGen.GenerateOctaves(this->gravelNoise,
		Vec3{double(chunk.cpos.x* CHUNK_WIDTH), 109.0134, double(chunk.cpos.z* CHUNK_WIDTH)},
		Int32_3{ 16, 1, 16 },
		Vec3{ oneThirtySecond, 1.0, oneThirtySecond }
	);
	this->stoneNoiseGen.GenerateOctaves(this->stoneNoise,
		Vec3{double(chunk.cpos.x* CHUNK_WIDTH), double(chunk.cpos.z* CHUNK_WIDTH), 0.0},
		Int32_3{ 16, 16, 1 },
		Vec3{ oneThirtySecond * 2.0, oneThirtySecond * 2.0, oneThirtySecond * 2.0 }
	);

	// Iterate through entire chunk
	for (int32_t x = 0; x < CHUNK_WIDTH; ++x) {
		for (int32_t z = 0; z < CHUNK_WIDTH; ++z) {
			size_t bindex = size_t(x * CHUNK_WIDTH + z);
			// Get values from noise maps
			Biome biome = biomeMap[bindex];
			bool sandActive = this->sandNoise[bindex] + this->rand.nextDouble() * 0.2 > 0.0;
			bool gravelActive = this->gravelNoise[bindex] + this->rand.nextDouble() * 0.2 > 3.0;
			int32_t stoneActive = Java::DoubleToInt32(this->stoneNoise[bindex] / 3.0 + 3.0 + this->rand.nextDouble() * 0.25);
			int32_t stoneDepth = -1;
			// Get biome-appropriate top and filler blocks
			BlockType topBlock = GetTopBlock(biome);
			BlockType fillerBlock = GetFillerBlock(biome);

			// Iterate over column top to bottom
			for (int32_t y = CHUNK_HEIGHT - 1; y >= 0; --y) {
				Int3 bpos{ x,y,z };
				// Place Bedrock at bottom with some randomness
				if (y <= 0 + this->rand.nextInt(5)) {
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
				if (currentBlock == BLOCK_STONE) {
					if (stoneDepth == -1) {
						if (stoneActive <= 0) {
							topBlock = BLOCK_AIR;
							fillerBlock = BLOCK_STONE;
						}
						else if (y >= WATER_LEVEL - 4 && y <= WATER_LEVEL + 1) {
							// If we're close to the water level, apply gravel and sand
							topBlock = GetTopBlock(biome);
							fillerBlock = GetFillerBlock(biome);

							if (gravelActive)
								topBlock = BLOCK_AIR;
							if (gravelActive)
								fillerBlock = BLOCK_GRAVEL;
							if (sandActive)
								topBlock = BLOCK_SAND;
							if (sandActive)
								fillerBlock = BLOCK_SAND;
						}

						// Add water if we're below water level
						if (y < WATER_LEVEL && topBlock == BLOCK_AIR) {
							topBlock = BLOCK_WATER_STILL;
						}

						stoneDepth = stoneActive;
						// Place filler block if we're underwater
						if (y >= WATER_LEVEL - 1) {
							chunk.setBlock(bpos, topBlock);
						}
						else {
							chunk.setBlock(bpos, fillerBlock);
						}
					}
					else if (stoneDepth > 0) {
						--stoneDepth;
						chunk.setBlock(bpos, fillerBlock);
						if (stoneDepth == 0 && fillerBlock == BLOCK_SAND) {
							stoneDepth = this->rand.nextInt(4);
							fillerBlock = BLOCK_SANDSTONE;
						}
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
void Generator::GenerateTerrain(Chunk& chunk) {
	const Int3 max{
		CHUNK_WIDTH / 4 + 1,
		CHUNK_HEIGHT / 8 + 1,
		CHUNK_WIDTH / 4 + 1
	};

	// Generate 4x16x4 low resolution noise map
	this->GenerateTerrainNoise(this->terrainNoiseField, Int3{chunk.cpos.x * 4, 0, chunk.cpos.z * 4}, max);

	// Terrain noise is interpolated and only sampled every 4 blocks
	for (int32_t sampleX = 0; sampleX < 4; ++sampleX) {
		for (int32_t sampleZ = 0; sampleZ < 4; ++sampleZ) {
			for (int32_t sampleY = 0; sampleY < 16; ++sampleY) {
				double verticalLerpStep = 0.125;

				// Get noise cube corners
				double corner000 = this->terrainNoiseField[size_t(((sampleX + 0) * max.z + sampleZ + 0) * max.y + sampleY + 0)];
				double corner010 = this->terrainNoiseField[size_t(((sampleX + 0) * max.z + sampleZ + 1) * max.y + sampleY + 0)];
				double corner100 = this->terrainNoiseField[size_t(((sampleX + 1) * max.z + sampleZ + 0) * max.y + sampleY + 0)];
				double corner110 = this->terrainNoiseField[size_t(((sampleX + 1) * max.z + sampleZ + 1) * max.y + sampleY + 0)];
				double corner001 = (this->terrainNoiseField[size_t(((sampleX + 0) * max.z + sampleZ + 0) * max.y + sampleY + 1)] - corner000) * verticalLerpStep;
				double corner011 = (this->terrainNoiseField[size_t(((sampleX + 0) * max.z + sampleZ + 1) * max.y + sampleY + 1)] - corner010) * verticalLerpStep;
				double corner101 = (this->terrainNoiseField[size_t(((sampleX + 1) * max.z + sampleZ + 0) * max.y + sampleY + 1)] - corner100) * verticalLerpStep;
				double corner111 = (this->terrainNoiseField[size_t(((sampleX + 1) * max.z + sampleZ + 1) * max.y + sampleY + 1)] - corner110) * verticalLerpStep;

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

							// If water is too cold, turn into ice
							double temp = this->temperature[size_t((sampleX * 4 + subX) * 16 + sampleZ * 4 + subZ)];
							int32_t yLevel = sampleY * 8 + subY;
							if (yLevel < WATER_LEVEL) {
								if (temp < 0.5 && yLevel >= WATER_LEVEL - 1) {
									blockType = BLOCK_ICE;
								}
								else {
									blockType = BLOCK_WATER_STILL;
								}
							}

							// If the terrain density falls below,
							// replace block with stone
							if (terrainDensity > 0.0) {
								blockType = BLOCK_STONE;
							}

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
void Generator::GenerateTerrainNoise(std::vector<double>& terrainMap, Int3 cpos, Int3 max) {
	terrainMap.resize(size_t(max.x * max.y * max.z), 0.0);

	double horiScale = 684.412;
	double vertScale = 684.412;

	// We do this to need to generate noise as often
	this->continentalnessNoiseGen.GenerateOctaves(this->continentalnessNoiseField,
		Int32_2{cpos.x, cpos.z},
		Int32_2{ max.x, max.z },
		Vec2{ 1.121, 1.121 },
		0.5
	);
	this->depthNoiseGen.GenerateOctaves(this->depthNoiseField,
		Int32_2{cpos.x, cpos.z},
		Int32_2{ max.x, max.z },
		Vec2{ 200.0, 200.0 },
		0.5
	);
	this->selectorNoiseGen.GenerateOctaves(this->selectorNoiseField,
		Vec3{double(cpos.x), double(cpos.y), double(cpos.z)},
		max,
		Vec3{ horiScale / 80.0, vertScale / 160.0, horiScale / 80.0 }
	);
	this->lowNoiseGen.GenerateOctaves(this->lowNoiseField,
		Vec3{double(cpos.x), double(cpos.y), double(cpos.z)},
		max,
		Vec3{ horiScale, vertScale, horiScale }
	);
	this->highNoiseGen.GenerateOctaves(this->highNoiseField,
		Vec3{double(cpos.x), double(cpos.y), double(cpos.z)},
		max,
		Vec3{ horiScale, vertScale, horiScale }
	);
	// Used to iterate 3D noise maps (low, high, selector)
	size_t xyzIndex = 0;
	// Used to iterate 2D Noise maps (depth, continentalness)
	size_t xzIndex = 0;
	int32_t scaleFraction = 16 / max.x;

	for (int32_t iX = 0; iX < max.x; ++iX) {
		int32_t sampleX = iX * scaleFraction + scaleFraction / 2;

		for (int32_t iZ = 0; iZ < max.z; ++iZ) {
			// Sample 2D noises
			int32_t sampleZ = iZ * scaleFraction + scaleFraction / 2;
			// Apply biome-noise-dependent variety
			size_t sample_index = size_t(sampleX * CHUNK_WIDTH + sampleZ);
			double temp = this->temperature[sample_index];
			double humi = this->humidity[sample_index] * temp;
			humi = 1.0 - humi;
			humi *= humi;
			humi *= humi;
			humi = 1.0 - humi;
			// Sample contientalness noise
			double continentalness = (this->continentalnessNoiseField[xzIndex] + 256.0) / 512.0;
			continentalness *= humi;
			if (continentalness > 1.0)
				continentalness = 1.0;
			// Sample depth noise
			double depthNoise = this->depthNoiseField[xzIndex] / 8000.0;
			if (depthNoise < 0.0)
				depthNoise = -depthNoise * 0.3;
			depthNoise = depthNoise * 3.0 - 2.0;
			if (depthNoise < 0.0) {
				depthNoise /= 2.0;
				if (depthNoise < -1.0)
					depthNoise = -1.0;
				depthNoise /= 1.4;
				depthNoise /= 2.0;
				continentalness = 0.0;
			}
			else {
				if (depthNoise > 1.0)
					depthNoise = 1.0;
				depthNoise /= 8.0;
			}
			if (continentalness < 0.0)
				continentalness = 0.0;
			continentalness += 0.5;
			depthNoise = depthNoise * double(max.y) / 16.0;
			double elevationOffset = double(max.y) / 2.0 + depthNoise * 4.0;
			++xzIndex;

			for (int32_t iY = 0; iY < max.y; ++iY) {
				// Sample 3D noises
				double terrainDensity = 0.0;
				double densityOffset = (double(iY) - elevationOffset) * 12.0 / continentalness;
				if (densityOffset < 0.0) {
					densityOffset *= 4.0;
				}
				// Sample low noise
				double lowNoise = this->lowNoiseField[xyzIndex] / 512.0;
				// Sample high noise
				double highNoise = this->highNoiseField[xyzIndex] / 512.0;
				// Sample selector noise
				double selectorNoise = (this->selectorNoiseField[xyzIndex] / 10.0 + 1.0) / 2.0;
				if (selectorNoise < 0.0) {
					terrainDensity = lowNoise;
				}
				else if (selectorNoise > 1.0) {
					terrainDensity = highNoise;
				}
				else {
					terrainDensity = lowNoise + (highNoise - lowNoise) * selectorNoise;
				}

				terrainDensity -= densityOffset;
				// Reduce density towards max height
				if (iY > max.y - 4) {
					double heightEdgeFade = double(float(iY - (max.y - 4)) / 3.0F);
					terrainDensity = (terrainDensity * (1.0 - heightEdgeFade)) + (-10.0 * heightEdgeFade);
				}

				terrainMap[xyzIndex] = terrainDensity;
				++xyzIndex;
			}
		}
	}
}

/**
 * @brief Probes the biome map at the specified coordinates
 *
 * @param worldPos The x,z coordinate of the desired block column
 * @return The Biome at that column
 */
Biome Generator::GetBiomeAt(Int2 worldPos) {
	// biomeMap is always for the chunk whose origin is (cpos.x*16, cpos.z*16).
	// Convert world coords to chunk-local [0,15] and index directly.
	int32_t localX = ((worldPos.x % CHUNK_WIDTH) + CHUNK_WIDTH) % CHUNK_WIDTH;
	int32_t localZ = ((worldPos.z % CHUNK_WIDTH) + CHUNK_WIDTH) % CHUNK_WIDTH;
	return biomeMap[size_t(localX * CHUNK_WIDTH + localZ)];
}


// Exact port of BiomeGenBase.getRandomWorldGenForTrees() and per-biome overrides.
void Generator::GenerateTreeForBiome(WorldManager& world, Java::Random& rand, Int3 pos, Biome biome) {
	switch (biome) {
	case BIOME_TAIGA:
		// Java: nextInt(3)==0 ? new WorldGenTaiga1() : new WorldGenTaiga2()
		// betrock++:
		//   TaigaTreeGenerator    has the WorldGenTaiga1 RNG sequence (nextInt(5)+7 height,
		//                                                              3 RNG, narrow spruce)
		//   AltTaigaTreeGenerator has the WorldGenTaiga2 RNG sequence (nextInt(4)+6 height,
		//                                                              5 RNG, bushy pine)
		if (rand.nextInt(3) == 0)
			TaigaTreeGenerator().Generate(world, rand, pos);     // = WorldGenTaiga1
		else
			AltTaigaTreeGenerator().Generate(world, rand, pos);  // = WorldGenTaiga2
		break;

	case BIOME_FOREST:
		// nextInt(5)==0 -> birch, else nextInt(3)==0 -> BigTree, else oak
		if (rand.nextInt(5) == 0) {
			// WorldGenForest (birch): same as oak but birch=true adds 1 to height base
			TreeGenerator().Generate(world, rand, pos, true);
		}
		else if (rand.nextInt(3) == 0) {
			BigTreeGenerator big;
			big.Configure(1.0, 1.0, 1.0);
			big.Generate(world, rand, pos);
		}
		else {
			TreeGenerator().Generate(world, rand, pos);
		}
		break;

	case BIOME_RAINFOREST:
		// nextInt(3)==0 -> BigTree, else oak
		if (rand.nextInt(3) == 0) {
			BigTreeGenerator big;
			big.Configure(1.0, 1.0, 1.0);
			big.Generate(world, rand, pos);
		}
		else {
			TreeGenerator().Generate(world, rand, pos);
		}
		break;

	default:
		// nextInt(10)==0 -> BigTree, else oak
		if (rand.nextInt(10) == 0) {
			BigTreeGenerator big;
			big.Configure(1.0, 1.0, 1.0);
			big.Generate(world, rand, pos);
		}
		else {
			TreeGenerator().Generate(world, rand, pos);
		}
		break;
	}
}


/**
 * @brief Populates the specified chunk with biome-specific features.
 *
 * Direct port of ChunkProviderGenerate.populate() from Beta 1.7.3.
 * Biome is sampled at blockX+16, blockZ+16 from stored chunk climate data.
 * RNG seeding, section order, rand call counts, and coordinate offsets all
 * match the Java source exactly.
 */
bool Generator::PopulateChunk(Chunk& chunk, WorldManager& world) {
	const int32_t blockX = chunk.cpos.x * CHUNK_WIDTH;
	const int32_t blockZ = chunk.cpos.z * CHUNK_WIDTH;

	// Java: getBiomeGenAt(blockX+16, blockZ+16) — samples the +X+Z corner chunk.
	// Read temperature/humidity from the stored chunk data and derive biome directly.
	auto getBiomeAt = [&](int wx, int wz) -> Biome {
		double temp = world.getTemperatureAt(wx, wz);
		double humi = world.getHumidityAt(wx, wz) * temp; // humidity *= temp as in Java
		if (temp < 0.1)                              return BIOME_TUNDRA;
		if (humi < 0.2) {
			if (temp < 0.5)                          return BIOME_TUNDRA;
			if (temp < 0.95)                         return BIOME_SAVANNA;
			return BIOME_DESERT;
		}
		if (humi > 0.5 && temp < 0.7)               return BIOME_SWAMPLAND;
		if (temp < 0.5)                              return BIOME_TAIGA;
		if (temp < 0.97)
			return (humi < 0.35) ? BIOME_SHRUBLAND : BIOME_FOREST;
		if (humi < 0.45)                             return BIOME_PLAINS;
		if (humi < 0.9)                              return BIOME_SEASONALFOREST;
		return BIOME_RAINFOREST;
		};
	Biome biome = getBiomeAt(blockX + CHUNK_WIDTH, blockZ + CHUNK_WIDTH);

	// Java RNG seeding sequence
	this->rand.setSeed(world.seed);
	int64_t xSalt = this->rand.nextLong() / 2L * 2L + 1L;
	int64_t zSalt = this->rand.nextLong() / 2L * 2L + 1L;
	this->rand.setSeed((int64_t(chunk.cpos.x) * xSalt + int64_t(chunk.cpos.z) * zSalt) ^ world.seed);

	Int3 coord;

	// Water lakes
	if (this->rand.nextInt(4) == 0) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
		coord.y = this->rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
		FeatureGenerator(BLOCK_WATER_STILL).GenerateLake(world, this->rand, coord);
	}

	// Lava lakes — nextInt(10) is always consumed when y >= WATER_LEVEL
	if (this->rand.nextInt(8) == 0) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
		coord.y = this->rand.nextInt(this->rand.nextInt(120) + 8);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
		if (coord.y < WATER_LEVEL || this->rand.nextInt(10) == 0)
			FeatureGenerator(BLOCK_LAVA_STILL).GenerateLake(world, this->rand, coord);
	}

	// Dungeons
	for (int32_t i = 0; i < 8; ++i) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
		coord.y = this->rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
		FeatureGenerator().GenerateDungeon(world, this->rand, coord);
	}

	// Clay (no +8)
	for (int32_t i = 0; i < 10; ++i) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH);
		coord.y = this->rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH);
		FeatureGenerator().GenerateClay(world, this->rand, coord, 32);
	}

	// Dirt blobs (no +8)
	for (int32_t i = 0; i < 20; ++i) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH);
		coord.y = this->rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH);
		FeatureGenerator(BLOCK_DIRT).GenerateMinable(world, this->rand, coord, 32);
	}

	// Gravel blobs (no +8)
	for (int32_t i = 0; i < 10; ++i) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH);
		coord.y = this->rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH);
		FeatureGenerator(BLOCK_GRAVEL).GenerateMinable(world, this->rand, coord, 32);
	}

	// Coal
	for (int32_t i = 0; i < 20; ++i) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH);
		coord.y = this->rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH);
		FeatureGenerator(BLOCK_ORE_COAL).GenerateMinable(world, this->rand, coord, 16);
	}

	// Iron (below y=64)
	for (int32_t i = 0; i < 20; ++i) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH);
		coord.y = this->rand.nextInt(CHUNK_HEIGHT / 2);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH);
		FeatureGenerator(BLOCK_ORE_IRON).GenerateMinable(world, this->rand, coord, 8);
	}

	// Gold (below y=32)
	for (int32_t i = 0; i < 2; ++i) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH);
		coord.y = this->rand.nextInt(CHUNK_HEIGHT / 4);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH);
		FeatureGenerator(BLOCK_ORE_GOLD).GenerateMinable(world, this->rand, coord, 8);
	}

	// Redstone (below y=16)
	for (int32_t i = 0; i < 8; ++i) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH);
		coord.y = this->rand.nextInt(CHUNK_HEIGHT / 8);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH);
		FeatureGenerator(BLOCK_ORE_REDSTONE_OFF).GenerateMinable(world, this->rand, coord, 7);
	}

	// Diamond (below y=16)
	{
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH);
		coord.y = this->rand.nextInt(CHUNK_HEIGHT / 8);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH);
		FeatureGenerator(BLOCK_ORE_DIAMOND).GenerateMinable(world, this->rand, coord, 7);
	}

	// Lapis lazuli — two independent nextInt(16) rolls (triangular distribution ~y=16)
	{
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH);
		coord.y = this->rand.nextInt(CHUNK_HEIGHT / 8) + this->rand.nextInt(CHUNK_HEIGHT / 8);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH);
		FeatureGenerator(BLOCK_ORE_LAPIS_LAZULI).GenerateMinable(world, this->rand, coord, 6);
	}

	// Tree count — noise at blockX*0.5, blockZ*0.5 (matches Java's var11=0.5 scale)
	double noiseVal = treeDensityNoiseGen.GenerateOctaves({ double(blockX) * 0.5, double(blockZ) * 0.5 });
	int32_t baseTreeCount = Java::DoubleToInt32((noiseVal / 8.0 + this->rand.nextDouble() * 4.0 + 4.0) / 3.0);
	int32_t treeCount = 0;
	if (this->rand.nextInt(10) == 0) ++treeCount;

	// Biome tree adjustments — additive (not else-if), matching Java exactly
	if (biome == BIOME_FOREST)        treeCount += baseTreeCount + 5;
	if (biome == BIOME_RAINFOREST)    treeCount += baseTreeCount + 5;
	if (biome == BIOME_SEASONALFOREST)treeCount += baseTreeCount + 2;
	if (biome == BIOME_TAIGA)         treeCount += baseTreeCount + 5;
	if (biome == BIOME_DESERT)        treeCount -= 20;
	if (biome == BIOME_TUNDRA)        treeCount -= 20;
	if (biome == BIOME_PLAINS)        treeCount -= 20;

	for (int32_t i = 0; i < treeCount; ++i) {
		int32_t tx = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
		int32_t tz = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
		int32_t ty = world.getHeightValue(tx, tz);
		coord = { tx, ty, tz };
		GenerateTreeForBiome(world, this->rand, coord, biome);
	}

	// Dandelions
	{
		int32_t count = 0;
		if (biome == BIOME_FOREST)         count = 2;
		if (biome == BIOME_SEASONALFOREST) count = 4;
		if (biome == BIOME_TAIGA)          count = 2;
		if (biome == BIOME_PLAINS)         count = 3;
		for (int32_t i = 0; i < count; ++i) {
			coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
			coord.y = this->rand.nextInt(CHUNK_HEIGHT);
			coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
			FeatureGenerator(BLOCK_DANDELION).GenerateFlowers(world, this->rand, coord);
		}
	}

	// Tall grass / ferns — rainforest nextInt(3) consumed inside loop
	{
		int32_t count = 0;
		if (biome == BIOME_FOREST)         count = 2;
		if (biome == BIOME_RAINFOREST)     count = 10;
		if (biome == BIOME_SEASONALFOREST) count = 2;
		if (biome == BIOME_TAIGA)          count = 1;
		if (biome == BIOME_PLAINS)         count = 10;
		for (int32_t i = 0; i < count; ++i) {
			int8_t grassMeta = 1;
			if (biome == BIOME_RAINFOREST && this->rand.nextInt(3) != 0)
				grassMeta = 2; // fern
			coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
			coord.y = this->rand.nextInt(CHUNK_HEIGHT);
			coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
			FeatureGenerator(BLOCK_TALLGRASS, grassMeta).GenerateTallgrass(world, this->rand, coord);
		}
	}

	// Dead bush (desert only)
	{
		int32_t count = (biome == BIOME_DESERT) ? 2 : 0;
		for (int32_t i = 0; i < count; ++i) {
			coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
			coord.y = this->rand.nextInt(CHUNK_HEIGHT);
			coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
			FeatureGenerator(BLOCK_DEADBUSH).GenerateDeadbush(world, this->rand, coord);
		}
	}

	// Rose (1-in-2)
	if (this->rand.nextInt(2) == 0) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
		coord.y = this->rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
		FeatureGenerator(BLOCK_ROSE).GenerateFlowers(world, this->rand, coord);
	}

	// Brown mushroom (1-in-4)
	if (this->rand.nextInt(4) == 0) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
		coord.y = this->rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
		FeatureGenerator(BLOCK_MUSHROOM_BROWN).GenerateFlowers(world, this->rand, coord);
	}

	// Red mushroom (1-in-8)
	if (this->rand.nextInt(8) == 0) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
		coord.y = this->rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
		FeatureGenerator(BLOCK_MUSHROOM_RED).GenerateFlowers(world, this->rand, coord);
	}

	// Sugar cane (10 attempts)
	for (int32_t i = 0; i < 10; ++i) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
		coord.y = this->rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
		FeatureGenerator().GenerateSugarcane(world, this->rand, coord);
	}

	// Pumpkins (1-in-32)
	if (this->rand.nextInt(32) == 0) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
		coord.y = this->rand.nextInt(CHUNK_HEIGHT);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
		FeatureGenerator().GeneratePumpkins(world, this->rand, coord);
	}

	// Cacti (desert only)
	{
		int32_t count = (biome == BIOME_DESERT) ? 10 : 0;
		for (int32_t i = 0; i < count; ++i) {
			coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
			coord.y = this->rand.nextInt(CHUNK_HEIGHT);
			coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
			FeatureGenerator().GenerateCacti(world, this->rand, coord);
		}
	}

	// Water springs (50 attempts)
	for (int32_t i = 0; i < 50; ++i) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
		coord.y = this->rand.nextInt(this->rand.nextInt(120) + 8);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
		FeatureGenerator(BLOCK_WATER_FLOWING).GenerateLiquid(world, this->rand, coord);
	}

	// Lava springs (20 attempts)
	for (int32_t i = 0; i < 20; ++i) {
		coord.x = blockX + this->rand.nextInt(CHUNK_WIDTH) + 8;
		coord.y = this->rand.nextInt(this->rand.nextInt(this->rand.nextInt(112) + 8) + 8);
		coord.z = blockZ + this->rand.nextInt(CHUNK_WIDTH) + 8;
		FeatureGenerator(BLOCK_LAVA_FLOWING).GenerateLiquid(world, this->rand, coord);
	}

	// Snow/ice — iterate blockX+8 to blockX+8+16 matching Java's region offset.
	// Java uses getTopSolidOrLiquidBlock here (skips non-solid like leaves) — NOT
	// the heightmap (which would stop at leaves). Use findTopSolidBlock to match.
	for (int32_t x = blockX + 8; x < blockX + 8 + CHUNK_WIDTH; ++x) {
		for (int32_t z = blockZ + 8; z < blockZ + 8 + CHUNK_WIDTH; ++z) {
			int32_t topY = world.findTopSolidBlock(x, z);
			double  temp = world.getTemperatureAt(x, z)
				- double(topY - 64) / 64.0 * 0.3;
			if (temp < 0.5 && topY > 0 && topY < CHUNK_HEIGHT
				&& world.getBlockId({ x, topY,   z }) == BLOCK_AIR
				&& world.getBlockId({ x, topY - 1, z }) != BLOCK_ICE
				&& IsSolid(world.getBlockId({ x, topY - 1, z }))) {
				world.setBlock({ x, topY, z }, BLOCK_SNOW_LAYER);
			}
		}
	}

	return true;
}