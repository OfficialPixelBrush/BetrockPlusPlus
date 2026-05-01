/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "feature_gen.h"
#include <algorithm>

// =============================================================================
//  GenerateLake
// =============================================================================
bool FeatureGenerator::GenerateLake(WorldManager& world, Java::Random& rand, Int3 pos) {
	pos.x -= 8;
	pos.z -= 8;

	// Sink to first non-air block
	while (pos.y > 0 && world.getBlockId({ pos.x, pos.y, pos.z }) == BLOCK_AIR)
		--pos.y;

	pos.y -= 4;

	bool shapeMask[2048] = {};
	int32_t blobCount = rand.nextInt(4) + 4;

	for (int32_t blobIndex = 0; blobIndex < blobCount; ++blobIndex) {
		double radX = rand.nextDouble() * 6.0 + 3.0;
		double radY = rand.nextDouble() * 4.0 + 2.0;
		double radZ = rand.nextDouble() * 6.0 + 3.0;
		double cx = rand.nextDouble() * (16.0 - radX - 2.0) + 1.0 + radX / 2.0;
		double cy = rand.nextDouble() * (8.0 - radY - 4.0) + 2.0 + radY / 2.0;
		double cz = rand.nextDouble() * (16.0 - radZ - 2.0) + 1.0 + radZ / 2.0;

		for (int32_t x = 1; x < 15; ++x)
			for (int32_t z = 1; z < 15; ++z)
				for (int32_t y = 1; y < 7; ++y) {
					double dx = (double(x) - cx) / (radX / 2.0);
					double dy = (double(y) - cy) / (radY / 2.0);
					double dz = (double(z) - cz) / (radZ / 2.0);
					if (dx * dx + dy * dy + dz * dz < 1.0)
						shapeMask[(x * 16 + z) * 8 + y] = true;
				}
	}

	// Reject if edges touch existing liquid (above waterline) or non-solid/wrong block (below)
	for (int32_t x = 0; x < 16; ++x)
		for (int32_t z = 0; z < 16; ++z)
			for (int32_t y = 0; y < 8; ++y) {
				bool edge = !shapeMask[(x * 16 + z) * 8 + y]
					&& ((x < 15 && shapeMask[((x + 1) * 16 + z) * 8 + y])
						|| (x > 0 && shapeMask[((x - 1) * 16 + z) * 8 + y])
						|| (z < 15 && shapeMask[(x * 16 + z + 1) * 8 + y])
						|| (z > 0 && shapeMask[(x * 16 + z - 1) * 8 + y])
						|| (y < 7 && shapeMask[(x * 16 + z) * 8 + y + 1])
						|| (y > 0 && shapeMask[(x * 16 + z) * 8 + y - 1]));
				if (!edge) continue;
				BlockType bt = world.getBlockId({ pos.x + x, pos.y + y, pos.z + z });
				if (y >= 4 && IsLiquid(bt))                              return false;
				if (y < 4 && !IsSolid(bt) && bt != this->type)          return false;
			}

	// Fill
	for (int32_t x = 0; x < 16; ++x)
		for (int32_t z = 0; z < 16; ++z)
			for (int32_t y = 0; y < 8; ++y)
				if (shapeMask[(x * 16 + z) * 8 + y])
					world.setBlock({ pos.x + x, pos.y + y, pos.z + z },
						y >= 4 ? BLOCK_AIR : this->type);

	// Exposed dirt -> grass
	for (int32_t x = 0; x < 16; ++x)
		for (int32_t z = 0; z < 16; ++z)
			for (int32_t y = 4; y < 8; ++y)
				if (shapeMask[(x * 16 + z) * 8 + y]
					&& world.getBlockId({ pos.x + x, pos.y + y - 1, pos.z + z }) == BLOCK_DIRT
					&& world.getSkyLight({ pos.x + x, pos.y + y, pos.z + z }) > 0)
					world.setBlock({ pos.x + x, pos.y + y - 1, pos.z + z }, BLOCK_GRASS);

	// Lava: solidify exposed edges
	if (this->type == BLOCK_LAVA_STILL || this->type == BLOCK_LAVA_FLOWING) {
		for (int32_t x = 0; x < 16; ++x)
			for (int32_t z = 0; z < 16; ++z)
				for (int32_t y = 0; y < 8; ++y) {
					bool edge = !shapeMask[(x * 16 + z) * 8 + y]
						&& ((x < 15 && shapeMask[((x + 1) * 16 + z) * 8 + y])
							|| (x > 0 && shapeMask[((x - 1) * 16 + z) * 8 + y])
							|| (z < 15 && shapeMask[(x * 16 + z + 1) * 8 + y])
							|| (z > 0 && shapeMask[(x * 16 + z - 1) * 8 + y])
							|| (y < 7 && shapeMask[(x * 16 + z) * 8 + y + 1])
							|| (y > 0 && shapeMask[(x * 16 + z) * 8 + y - 1]));
					if (edge && (y < 4 || rand.nextInt(2) != 0)
						&& IsSolid(world.getBlockId({ pos.x + x, pos.y + y, pos.z + z })))
						world.setBlock({ pos.x + x, pos.y + y, pos.z + z }, BLOCK_STONE);
				}
	}
	return true;
}

// =============================================================================
//  GenerateDungeon
// =============================================================================
bool FeatureGenerator::GenerateDungeon(WorldManager& world, Java::Random& rand, Int3 pos) {
	const int8_t dungeonHeight = 3;
	int32_t dungeonWidthX = rand.nextInt(2) + 2;
	int32_t dungeonWidthZ = rand.nextInt(2) + 2;
	int32_t validEntries = 0;

	for (int32_t xi = pos.x - dungeonWidthX - 1; xi <= pos.x + dungeonWidthX + 1; ++xi)
		for (int32_t yi = pos.y - 1; yi <= pos.y + dungeonHeight + 1; ++yi)
			for (int32_t zi = pos.z - dungeonWidthZ - 1; zi <= pos.z + dungeonWidthZ + 1; ++zi) {
				BlockType bt = world.getBlockId({ xi, yi, zi });
				if (yi == pos.y - 1 && !IsSolid(bt)) return false;
				if (yi == pos.y + dungeonHeight + 1 && !IsSolid(bt)) return false;
				bool isWall = (xi == pos.x - dungeonWidthX - 1 || xi == pos.x + dungeonWidthX + 1
					|| zi == pos.z - dungeonWidthZ - 1 || zi == pos.z + dungeonWidthZ + 1);
				if (isWall && yi == pos.y
					&& bt == BLOCK_AIR
					&& world.getBlockId({ xi, yi + 1, zi }) == BLOCK_AIR)
					++validEntries;
			}

	if (validEntries < 1 || validEntries > 5) return false;

	for (int32_t xi = pos.x - dungeonWidthX - 1; xi <= pos.x + dungeonWidthX + 1; ++xi)
		for (int32_t yi = pos.y + dungeonHeight; yi >= pos.y - 1; --yi)
			for (int32_t zi = pos.z - dungeonWidthZ - 1; zi <= pos.z + dungeonWidthZ + 1; ++zi) {
				bool interior = (xi != pos.x - dungeonWidthX - 1 && xi != pos.x + dungeonWidthX + 1
					&& yi != pos.y - 1 && yi != pos.y + dungeonHeight + 1
					&& zi != pos.z - dungeonWidthZ - 1 && zi != pos.z + dungeonWidthZ + 1);
				if (interior) {
					world.setBlock({ xi, yi, zi }, BLOCK_AIR);
				}
				else if (yi >= 0 && !IsSolid(world.getBlockId({ xi, yi - 1, zi }))) {
					world.setBlock({ xi, yi, zi }, BLOCK_AIR);
				}
				else if (IsSolid(world.getBlockId({ xi, yi, zi }))) {
					BlockType wall = (yi == pos.y - 1 && rand.nextInt(4) != 0)
						? BLOCK_COBBLESTONE_MOSSY : BLOCK_COBBLESTONE;
					world.setBlock({ xi, yi, zi }, wall);
				}
			}

	// Up to 2 chests, 3 placement attempts each
	for (int32_t chestAttempt = 0; chestAttempt < 2; ++chestAttempt) {
		for (int32_t attempt = 0; attempt < 3; ++attempt) {
			int32_t cx = pos.x + rand.nextInt(dungeonWidthX * 2 + 1) - dungeonWidthX;
			int32_t cz = pos.z + rand.nextInt(dungeonWidthZ * 2 + 1) - dungeonWidthZ;
			if (world.getBlockId({ cx, pos.y, cz }) != BLOCK_AIR) continue;
			int32_t adj = 0;
			if (IsSolid(world.getBlockId({ cx - 1, pos.y, cz }))) ++adj;
			if (IsSolid(world.getBlockId({ cx + 1, pos.y, cz }))) ++adj;
			if (IsSolid(world.getBlockId({ cx, pos.y, cz - 1 }))) ++adj;
			if (IsSolid(world.getBlockId({ cx, pos.y, cz + 1 }))) ++adj;
			if (adj == 1) {
				world.setBlock({ cx, pos.y, cz }, BLOCK_CHEST);
				// Consume loot RNG to stay seed-accurate even without tile entity support.
				// Java: for each of 8 attempts, pickCheckLootItem() consumes RNG, and
				// if non-null, an extra nextInt(getSizeInventory()) [27 for dungeon chest]
				// is consumed to pick the slot. Replicate exactly.
				for (int32_t slot = 0; slot < 8; ++slot) {
					if (GenerateDungeonChestLoot(rand) != 0)
						rand.nextInt(27);
				}
				break;
			}
		}
	}

	world.setBlock(pos, BLOCK_MOB_SPAWNER);
	PickMobToSpawn(rand); // consume RNG for mob type
	return true;
}

// =============================================================================
//  GenerateDungeonChestLoot — exact RNG port (all rand calls must fire)
// =============================================================================
int FeatureGenerator::GenerateDungeonChestLoot(Java::Random& rand) {
	int32_t roll = rand.nextInt(11);
	switch (roll) {
	case 0:  return 1;                                         // saddle
	case 1:  rand.nextInt(4); return 1;                        // iron ingot
	case 2:  return 1;                                         // bread
	case 3:  rand.nextInt(4); return 1;                        // wheat
	case 4:  rand.nextInt(4); return 1;                        // gunpowder
	case 5:  rand.nextInt(4); return 1;                        // string
	case 6:  return 1;                                         // bucket
	case 7:  if (rand.nextInt(100) == 0) return 1; break;      // golden apple 1%
	case 8:
		if (rand.nextInt(2) == 0) { rand.nextInt(4); return 1; }
		break;                                                 // redstone 50%
	case 9:
		if (rand.nextInt(10) == 0) { rand.nextInt(2); return 1; }
		break;                                                 // music disc 10%
	case 10: return 1;                                         // bone meal
	default: break;
	}
	return 0;
}

std::string FeatureGenerator::PickMobToSpawn(Java::Random& rand) {
	switch (rand.nextInt(4)) {
	case 0:         return "Skeleton";
	case 1: case 2: return "Zombie";
	case 3:         return "Spider";
	default:        return "Zombie";
	}
}

// =============================================================================
//  GenerateClay
// =============================================================================
bool FeatureGenerator::GenerateClay(WorldManager& world, Java::Random& rand, Int3 pos, int32_t blobSize) {
	BlockType at = world.getBlockId(pos);
	if (at != BLOCK_WATER_STILL && at != BLOCK_WATER_FLOWING) return false;

	float  angle = rand.nextFloat() * float(JavaMath::PI);
	double xStart = double(float(pos.x + 8) + MathHelper::sin(angle) * float(blobSize) / 8.0F);
	double xEnd = double(float(pos.x + 8) - MathHelper::sin(angle) * float(blobSize) / 8.0F);
	double zStart = double(float(pos.z + 8) + MathHelper::cos(angle) * float(blobSize) / 8.0F);
	double zEnd = double(float(pos.z + 8) - MathHelper::cos(angle) * float(blobSize) / 8.0F);
	double yStart = double(pos.y + rand.nextInt(3) + 2);
	double yEnd = double(pos.y + rand.nextInt(3) + 2);

	for (int32_t i = 0; i <= blobSize; ++i) {
		double xC = xStart + (xEnd - xStart) * double(i) / double(blobSize);
		double yC = yStart + (yEnd - yStart) * double(i) / double(blobSize);
		double zC = zStart + (zEnd - zStart) * double(i) / double(blobSize);
		double blobScale = rand.nextDouble() * double(blobSize) / 16.0;
		double radXZ = double(MathHelper::sin(float(i) * float(JavaMath::PI) / float(blobSize)) + 1.0F) * blobScale + 1.0;
		double radY = double(MathHelper::sin(float(i) * float(JavaMath::PI) / float(blobSize)) + 1.0F) * blobScale + 1.0;
		int32_t minX = MathHelper::floor_double(xC - radXZ / 2.0);
		int32_t maxX = MathHelper::floor_double(xC + radXZ / 2.0);
		int32_t minY = MathHelper::floor_double(yC - radY / 2.0);
		int32_t maxY = MathHelper::floor_double(yC + radY / 2.0);
		int32_t minZ = MathHelper::floor_double(zC - radXZ / 2.0);
		int32_t maxZ = MathHelper::floor_double(zC + radXZ / 2.0);
		for (int32_t x = minX; x <= maxX; ++x)
			for (int32_t y = minY; y <= maxY; ++y)
				for (int32_t z = minZ; z <= maxZ; ++z) {
					double dx = (double(x) + 0.5 - xC) / (radXZ / 2.0);
					double dy = (double(y) + 0.5 - yC) / (radY / 2.0);
					double dz = (double(z) + 0.5 - zC) / (radXZ / 2.0);
					if (dx * dx + dy * dy + dz * dz < 1.0 && world.getBlockId({ x,y,z }) == BLOCK_SAND)
						world.setBlock({ x,y,z }, BLOCK_CLAY);
				}
	}
	return true;
}

// =============================================================================
//  GenerateMinable
// =============================================================================
bool FeatureGenerator::GenerateMinable(WorldManager& world, Java::Random& rand, Int3 pos, int32_t blobSize) {
	float  angle = rand.nextFloat() * float(JavaMath::PI);
	double xStart = double(float(pos.x + 8) + MathHelper::sin(angle) * float(blobSize) / 8.0F);
	double xEnd = double(float(pos.x + 8) - MathHelper::sin(angle) * float(blobSize) / 8.0F);
	double zStart = double(float(pos.z + 8) + MathHelper::cos(angle) * float(blobSize) / 8.0F);
	double zEnd = double(float(pos.z + 8) - MathHelper::cos(angle) * float(blobSize) / 8.0F);
	double yStart = double(pos.y + rand.nextInt(3) + 2);
	double yEnd = double(pos.y + rand.nextInt(3) + 2);

	for (int32_t i = 0; i <= blobSize; ++i) {
		double xC = xStart + (xEnd - xStart) * double(i) / double(blobSize);
		double yC = yStart + (yEnd - yStart) * double(i) / double(blobSize);
		double zC = zStart + (zEnd - zStart) * double(i) / double(blobSize);
		double blobScale = rand.nextDouble() * double(blobSize) / 16.0;
		double radXZ = double(MathHelper::sin(float(i) * float(JavaMath::PI) / float(blobSize)) + 1.0F) * blobScale + 1.0;
		double radY = double(MathHelper::sin(float(i) * float(JavaMath::PI) / float(blobSize)) + 1.0F) * blobScale + 1.0;
		int32_t minX = MathHelper::floor_double(xC - radXZ / 2.0);
		int32_t maxX = MathHelper::floor_double(xC + radXZ / 2.0);
		int32_t minY = MathHelper::floor_double(yC - radY / 2.0);
		int32_t maxY = MathHelper::floor_double(yC + radY / 2.0);
		int32_t minZ = MathHelper::floor_double(zC - radXZ / 2.0);
		int32_t maxZ = MathHelper::floor_double(zC + radXZ / 2.0);
		for (int32_t x = minX; x <= maxX; ++x) {
			double dx = (double(x) + 0.5 - xC) / (radXZ / 2.0);
			if (dx * dx >= 1.0) continue;
			for (int32_t y = minY; y <= maxY; ++y) {
				double dy = (double(y) + 0.5 - yC) / (radY / 2.0);
				if (dx * dx + dy * dy >= 1.0) continue;
				for (int32_t z = minZ; z <= maxZ; ++z) {
					double dz = (double(z) + 0.5 - zC) / (radXZ / 2.0);
					if (dx * dx + dy * dy + dz * dz < 1.0 && world.getBlockId({ x,y,z }) == BLOCK_STONE)
						world.setBlock({ x,y,z }, this->type);
				}
			}
		}
	}
	return true;
}

// =============================================================================
//  GenerateFlowers — 64 attempts
//  Flowers (dandelion/rose): needs GRASS below.
//  Mushrooms: needs solid below AND skylight == 0.
// =============================================================================
bool FeatureGenerator::GenerateFlowers(WorldManager& world, Java::Random& rand, Int3 pos) {
	bool isMushroom = (this->type == BLOCK_MUSHROOM_BROWN || this->type == BLOCK_MUSHROOM_RED);

	for (int32_t i = 0; i < 64; ++i) {
		int32_t x = pos.x + rand.nextInt(8) - rand.nextInt(8);
		int32_t y = pos.y + rand.nextInt(4) - rand.nextInt(4);
		int32_t z = pos.z + rand.nextInt(8) - rand.nextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT) continue;
		if (world.getBlockId({ x, y, z }) != BLOCK_AIR) continue;

		if (isMushroom) {
			// Java BlockMushroom.canBlockStay:
			//   world.canSeeSky(x, y, z) == false  (no direct sky access)
			//   && opaqueCubeLookup[blockBelow] (solid block below)
			// During worldgen sky light is not yet propagated, so skyLight == 0
			// is the correct proxy for canSeeSky == false.
			if (IsSolid(world.getBlockId({ x, y - 1, z }))
				&& world.getSkyLight({ x, y, z }) == 0)
				world.setBlock({ x, y, z }, this->type);
		}
		else {
			// Java BlockFlower.canThisPlantGrowOnThisBlockID: only GRASS.
			// (dirt and farmland are NOT accepted for worldgen flowers in b1.7.3)
			BlockType below = world.getBlockId({ x, y - 1, z });
			if (below == BLOCK_GRASS)
				world.setBlock({ x, y, z }, this->type);
		}
	}
	return true;
}

// =============================================================================
//  GenerateTallgrass — descend then 128 attempts
//  canBlockStay: grass or dirt below.
// =============================================================================
bool FeatureGenerator::GenerateTallgrass(WorldManager& world, Java::Random& rand, Int3 pos) {
	while (pos.y > 0) {
		BlockType b = world.getBlockId({ pos.x, pos.y, pos.z });
		if (b != BLOCK_AIR && b != BLOCK_LEAVES) break;
		--pos.y;
	}

	for (int32_t i = 0; i < 128; ++i) {
		int32_t x = pos.x + rand.nextInt(8) - rand.nextInt(8);
		int32_t y = pos.y + rand.nextInt(4) - rand.nextInt(4);
		int32_t z = pos.z + rand.nextInt(8) - rand.nextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT) continue;
		if (world.getBlockId({ x, y, z }) != BLOCK_AIR) continue;
		BlockType below = world.getBlockId({ x, y - 1, z });
		if (below == BLOCK_GRASS || below == BLOCK_DIRT)
			world.setBlock({ x, y, z }, this->type, uint8_t(this->meta));
	}
	return true;
}

// =============================================================================
//  GenerateDeadbush — descend then 4 attempts, sand below
// =============================================================================
bool FeatureGenerator::GenerateDeadbush(WorldManager& world, Java::Random& rand, Int3 pos) {
	while (pos.y > 0) {
		BlockType b = world.getBlockId({ pos.x, pos.y, pos.z });
		if (b != BLOCK_AIR && b != BLOCK_LEAVES) break;
		--pos.y;
	}

	for (int32_t i = 0; i < 4; ++i) {
		int32_t x = pos.x + rand.nextInt(8) - rand.nextInt(8);
		int32_t y = pos.y + rand.nextInt(4) - rand.nextInt(4);
		int32_t z = pos.z + rand.nextInt(8) - rand.nextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT) continue;
		if (world.getBlockId({ x, y, z }) == BLOCK_AIR
			&& world.getBlockId({ x, y - 1, z }) == BLOCK_SAND)
			world.setBlock({ x, y, z }, BLOCK_DEADBUSH);
	}
	return true;
}

// =============================================================================
//  GenerateSugarcane — 20 attempts, Y stays fixed, breaks on invalid placement
// =============================================================================
bool FeatureGenerator::GenerateSugarcane(WorldManager& world, Java::Random& rand, Int3 pos) {
	for (int32_t i = 0; i < 20; ++i) {
		int32_t x = pos.x + rand.nextInt(4) - rand.nextInt(4);
		int32_t y = pos.y; // Y is fixed across all attempts
		int32_t z = pos.z + rand.nextInt(4) - rand.nextInt(4);
		if (world.getBlockId({ x, y, z }) != BLOCK_AIR) continue;

		auto isWater = [&](int wx, int wy, int wz) {
			BlockType b = world.getBlockId({ wx, wy, wz });
			return b == BLOCK_WATER_STILL || b == BLOCK_WATER_FLOWING;
			};
		if (!isWater(x - 1, y - 1, z) && !isWater(x + 1, y - 1, z)
			&& !isWater(x, y - 1, z - 1) && !isWater(x, y - 1, z + 1)) continue;

		int32_t height = 2 + rand.nextInt(rand.nextInt(3) + 1);
		for (int32_t h = 0; h < height; ++h) {
			BlockType below = world.getBlockId({ x, y + h - 1, z });
			if (below != BLOCK_GRASS && below != BLOCK_DIRT
				&& below != BLOCK_SAND && below != BLOCK_SUGARCANE) break;
			if (world.getBlockId({ x, y + h, z }) != BLOCK_AIR) break;
			world.setBlock({ x, y + h, z }, BLOCK_SUGARCANE);
		}
	}
	return true;
}

// =============================================================================
//  GeneratePumpkins — 64 attempts, grass below, no adjacent pumpkins
//  Java: isAirBlock + grass below + canPlaceBlockAt. canPlaceBlockAt for pumpkin
//  checks no adjacent pumpkin on all 4 cardinal sides.
// =============================================================================
bool FeatureGenerator::GeneratePumpkins(WorldManager& world, Java::Random& rand, Int3 pos) {
	for (int32_t i = 0; i < 64; ++i) {
		int32_t x = pos.x + rand.nextInt(8) - rand.nextInt(8);
		int32_t y = pos.y + rand.nextInt(4) - rand.nextInt(4);
		int32_t z = pos.z + rand.nextInt(8) - rand.nextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT) continue;
		if (world.getBlockId({ x,   y,   z }) != BLOCK_AIR)     continue;
		if (world.getBlockId({ x,   y - 1, z }) != BLOCK_GRASS)   continue;
		// canPlaceBlockAt: no adjacent pumpkins on cardinal sides
		if (world.getBlockId({ x - 1, y, z }) == BLOCK_PUMPKIN)   continue;
		if (world.getBlockId({ x + 1, y, z }) == BLOCK_PUMPKIN)   continue;
		if (world.getBlockId({ x,   y, z - 1 }) == BLOCK_PUMPKIN)   continue;
		if (world.getBlockId({ x,   y, z + 1 }) == BLOCK_PUMPKIN)   continue;
		world.setBlock({ x, y, z }, BLOCK_PUMPKIN, uint8_t(rand.nextInt(4)));
	}
	return true;
}

// =============================================================================
//  GenerateCacti — 10 attempts, break per-height if placement invalid
// =============================================================================
bool FeatureGenerator::GenerateCacti(WorldManager& world, Java::Random& rand, Int3 pos) {
	for (int32_t i = 0; i < 10; ++i) {
		int32_t x = pos.x + rand.nextInt(8) - rand.nextInt(8);
		int32_t y = pos.y + rand.nextInt(4) - rand.nextInt(4);
		int32_t z = pos.z + rand.nextInt(8) - rand.nextInt(8);
		if (world.getBlockId({ x, y, z }) != BLOCK_AIR) continue;

		int32_t height = 1 + rand.nextInt(rand.nextInt(3) + 1);
		for (int32_t h = 0; h < height; ++h) {
			BlockType below = world.getBlockId({ x, y + h - 1, z });
			if (below != BLOCK_SAND && below != BLOCK_CACTUS)           break;
			if (world.getBlockId({ x,   y + h, z }) != BLOCK_AIR)      break;
			if (world.getBlockId({ x - 1, y + h, z }) != BLOCK_AIR)      break;
			if (world.getBlockId({ x + 1, y + h, z }) != BLOCK_AIR)      break;
			if (world.getBlockId({ x,   y + h, z - 1 }) != BLOCK_AIR)      break;
			if (world.getBlockId({ x,   y + h, z + 1 }) != BLOCK_AIR)      break;
			world.setBlock({ x, y + h, z }, BLOCK_CACTUS);
		}
	}
	return true;
}

// =============================================================================
//  GenerateLiquid — spring placement + lava updateTick RNG simulation
// =============================================================================
bool FeatureGenerator::GenerateLiquid(WorldManager& world, Java::Random& rand, Int3 pos) {
	if (world.getBlockId({ pos.x, pos.y + 1, pos.z }) != BLOCK_STONE) return false;
	if (world.getBlockId({ pos.x, pos.y - 1, pos.z }) != BLOCK_STONE) return false;
	BlockType cur = world.getBlockId(pos);
	if (cur != BLOCK_AIR && cur != BLOCK_STONE) return false;

	int32_t stone = 0, air = 0;
	if (world.getBlockId({ pos.x - 1, pos.y, pos.z }) == BLOCK_STONE) ++stone;
	if (world.getBlockId({ pos.x + 1, pos.y, pos.z }) == BLOCK_STONE) ++stone;
	if (world.getBlockId({ pos.x, pos.y, pos.z - 1 }) == BLOCK_STONE) ++stone;
	if (world.getBlockId({ pos.x, pos.y, pos.z + 1 }) == BLOCK_STONE) ++stone;
	if (world.getBlockId({ pos.x - 1, pos.y, pos.z }) == BLOCK_AIR)   ++air;
	if (world.getBlockId({ pos.x + 1, pos.y, pos.z }) == BLOCK_AIR)   ++air;
	if (world.getBlockId({ pos.x, pos.y, pos.z - 1 }) == BLOCK_AIR)   ++air;
	if (world.getBlockId({ pos.x, pos.y, pos.z + 1 }) == BLOCK_AIR)   ++air;

	if (stone == 3 && air == 1) {
		world.setBlock(pos, this->type);

		// Simulate BlockStationary.updateTick RNG consumption after spring placement.
		// Water (BlockStationary, material=water): updateTick checks material==lava
		// -> false -> does nothing. 0 rand calls consumed.
		//
		// Lava (BlockStationary, material=lava): exact port of the fire-spread path.
		//   var6 = rand.nextInt(3)  — number of fire-spread attempts (0, 1, or 2)
		//   for each attempt:
		//     x += rand.nextInt(3) - 1
		//     y += 1
		//     z += rand.nextInt(3) - 1
		//     if block at (x,y,z) == AIR: check neighbours for flammable -> place fire; return
		//     else if block is solid: return early
		//     else (non-solid non-air): continue to next attempt
		if (this->type == BLOCK_LAVA_STILL || this->type == BLOCK_LAVA_FLOWING) {
			int32_t attempts = rand.nextInt(3);
			int32_t fx = pos.x, fy = pos.y, fz = pos.z;
			for (int32_t attempt = 0; attempt < attempts; ++attempt) {
				fx += rand.nextInt(3) - 1;
				fy += 1;
				fz += rand.nextInt(3) - 1;
				BlockType fb = world.getBlockId({ fx, fy, fz });
				if (fb == BLOCK_AIR) {
					// Java checks neighbours for flammability then places fire.
					// No extra rand consumed — flammability is a pure table lookup.
					// Java then returns from updateTick.
					break;
				}
				else if (IsSolid(fb)) {
					// Java returns early — loop terminates, no more rand calls.
					break;
				}
				// Non-solid non-air: loop continues to next attempt.
			}
		}
	}
	return true;
}