/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "feature_gen.h"

/**
 * @brief Create a Beta 1.7.3 Feature Object
 * 
 * @param pBlocktype Block-ID that's used by some of the generators
 */
FeatureGenerator::FeatureGenerator(BlockType pType) {
	this->type = pType;
}

/**
 * @brief Create a Beta 1.7.3 Feature Object
 * 
 * @param pBlocktype Block-ID that's used by some of the generators
 * @param pMeta Meta value that's used by the placed block id
 */
FeatureGenerator::FeatureGenerator(BlockType pType, int8_t pMeta) {
	this->type = pType;
	this->meta = pMeta;
}

/**
 * @brief Generate a lake
 * 
 * @param world World in which the feature will attempt to generate
 * @param rand The random object for this feature
 * @param pos Initial position of the feature
 * @return If generation succeeded 
 */
bool FeatureGenerator::GenerateLake(World *world, Java::Random& rand, Int3 pos) {
	pos.x -= 8;

	// Check for any non-air blocks
	for (pos.z -= 8; pos.y > 0; --pos.y) {
		if (world->GetBlockType(Int3{pos.x, pos.y, pos.z}) != BLOCK_AIR)
			break;
	}

	pos.y -= 4;
	bool shapeMask[2048] = {};
	int32_t blobCount = rand.nextInt(4) + 4;

	for (int32_t blobIndex = 0; blobIndex < blobCount; ++blobIndex) {
		double blobRadiusX = rand.nextDouble() * 6.0 + 3.0;
		double blobRadiusY = rand.nextDouble() * 4.0 + 2.0;
		double blobRadiusZ = rand.nextDouble() * 6.0 + 3.0;

		double blobCenterX = rand.nextDouble() * (16.0 - blobRadiusX - 2.0) + 1.0 + blobRadiusX / 2.0;
		double blobCenterY = rand.nextDouble() * (8.0 - blobRadiusY - 4.0) + 2.0 + blobRadiusY / 2.0;
		double blobCenterZ = rand.nextDouble() * (16.0 - blobRadiusZ - 2.0) + 1.0 + blobRadiusZ / 2.0;

		for (int32_t x = 1; x < 15; ++x) {
			for (int32_t z = 1; z < 15; ++z) {
				for (int32_t y = 1; y < 7; ++y) {
					double dx = (double(x) - blobCenterX) / (blobRadiusX / 2.0);
					double dy = (double(y) - blobCenterY) / (blobRadiusY / 2.0);
					double dz = (double(z) - blobCenterZ) / (blobRadiusZ / 2.0);
					double distance = dx * dx + dy * dy + dz * dz;
					if (distance < 1.0) {
						shapeMask[(x * 16 + z) * 8 + y] = true;
					}
				}
			}
		}
	}

	// Check if there's no other water nearby that'd intersect our new lake
	for (int32_t x = 0; x < 16; ++x) {
		for (int32_t z = 0; z < 16; ++z) {
			for (int32_t y = 0; y < 8; ++y) {
				bool edge =
					(!shapeMask[(x * 16 + z) * 8 + y]) && (((x < 15) && (shapeMask[((x + 1) * 16 + z) * 8 + y])) ||
														   ((x > 0) && (shapeMask[((x - 1) * 16 + z) * 8 + y])) ||
														   ((z < 15) && (shapeMask[(x * 16 + z + 1) * 8 + y])) ||
														   ((z > 0) && (shapeMask[(x * 16 + (z - 1)) * 8 + y])) ||
														   ((y < 7) && (shapeMask[(x * 16 + z) * 8 + y + 1])) ||
														   ((y > 0) && (shapeMask[(x * 16 + z) * 8 + (y - 1)])));
				if (edge) {
					BlockType blockType = world->GetBlockType(Int3{pos.x + x, pos.y + y, pos.z + z});
					if (y >= 4 && IsLiquid(blockType)) {
						return false;
					}
					if (y < 4 && !IsSolid(blockType) &&
						world->GetBlockType(Int3{pos.x + x, pos.y + y, pos.z + z}) != this->type) {
						return false;
					}
				}
			}
		}
	}

	// Fill the lake
	for (int32_t x = 0; x < 16; ++x) {
		for (int32_t z = 0; z < 16; ++z) {
			for (int32_t y = 0; y < 8; ++y) {
				if (shapeMask[(x * 16 + z) * 8 + y]) {
					world->SetBlockType(y >= 4 ? BLOCK_AIR : BlockType(this->type),
										Int3{pos.x + x, pos.y + y, pos.z + z});
				}
			}
		}
	}

	// Replace exposed dirt with grass
	for (int32_t x = 0; x < 16; ++x) {
		for (int32_t z = 0; z < 16; ++z) {
			for (int32_t y = 4; y < 8; ++y) {
				if (shapeMask[(x * 16 + z) * 8 + y] &&
					world->GetBlockType(Int3{pos.x + x, pos.y + y - 1, pos.z + z}) == BLOCK_DIRT &&
					world->GetSkyLight(Int3{pos.x + x, pos.y + y, pos.z + z}) > 0) {
					world->SetBlockType(BLOCK_GRASS, Int3{pos.x + x, pos.y + y - 1, pos.z + z});
				}
			}
		}
	}

	// If we're generating a lava lake, make the edges into stone
	if (this->type == BLOCK_LAVA_STILL || this->type == BLOCK_LAVA_FLOWING) {
		for (int32_t x = 0; x < 16; ++x) {
			for (int32_t z = 0; z < 16; ++z) {
				for (int32_t y = 0; y < 8; ++y) {
					bool edge =
						!(shapeMask[(x * 16 + z) * 8 + y]) && ((x < 15 && shapeMask[((x + 1) * 16 + z) * 8 + y]) ||
															   (x > 0 && shapeMask[((x - 1) * 16 + z) * 8 + y]) ||
															   (z < 15 && shapeMask[(x * 16 + z + 1) * 8 + y]) ||
															   (z > 0 && shapeMask[(x * 16 + (z - 1)) * 8 + y]) ||
															   (y < 7 && shapeMask[(x * 16 + z) * 8 + y + 1]) ||
															   (y > 0 && shapeMask[(x * 16 + z) * 8 + (y - 1)]));
					if (edge && (y < 4 || rand.nextInt(2) != 0) &&
						IsSolid(world->GetBlockType(Int3{pos.x + x, pos.y + y, pos.z + z}))) {
						world->SetBlockType(BLOCK_STONE, Int3{pos.x + x, pos.y + y, pos.z + z});
					}
				}
			}
		}
	}

	return true;
}

/**
 * @brief Generate a dungeon
 * 
 * @param world World in which the feature will attempt to generate
 * @param rand The random object for this feature
 * @param pos Initial position of the feature
 * @return If generation succeeded 
 */
bool FeatureGenerator::GenerateDungeon(World *world, Java::Random& rand, Int3 pos) {
	int8_t dungeonHeight = 3;
	int32_t dungeonWidthX = rand.nextInt(2) + 2;
	int32_t dungeonWidthZ = rand.nextInt(2) + 2;
	int32_t validEntires = 0;

	// Determine if a dungeon can be placed
	for (int32_t xI = pos.x - dungeonWidthX - 1; xI <= pos.x + dungeonWidthX + 1; ++xI) {
		for (int32_t yI = pos.y - 1; yI <= pos.y + dungeonHeight + 1; ++yI) {
			for (int32_t zI = pos.z - dungeonWidthZ - 1; zI <= pos.z + dungeonWidthZ + 1; ++zI) {
				BlockType blockType = world->GetBlockType(Int3{xI, yI, zI});

				// Floor and ceiling must be solid
				if (yI == pos.y - 1 && !IsSolid(blockType))
					return false;
				if (yI == pos.y + dungeonHeight + 1 && !IsSolid(blockType))
					return false;

				if ((xI == pos.x - dungeonWidthX - 1 || xI == pos.x + dungeonWidthX + 1 ||
					 zI == pos.z - dungeonWidthZ - 1 || zI == pos.z + dungeonWidthZ + 1) &&
					yI == pos.y && blockType == BLOCK_AIR && world->GetBlockType(Int3{xI, yI + 1, zI}) == BLOCK_AIR) {
					++validEntires;
				}
			}
		}
	}

	// There are too few or too many ways to get in, so we abort
	if (validEntires < 1 || validEntires > 5) {
		return false;
	}

	// Build the dungeon
	for (int32_t xI = pos.x - dungeonWidthX - 1; xI <= pos.x + dungeonWidthX + 1; ++xI) {
		for (int32_t yI = pos.y + dungeonHeight; yI >= pos.y - 1; --yI) {
			for (int32_t zI = pos.z - dungeonWidthZ - 1; zI <= pos.z + dungeonWidthZ + 1; ++zI) {
				// Check if the current block is not a wall
				if (xI != pos.x - dungeonWidthX - 1 && yI != pos.y - 1 && zI != pos.z - dungeonWidthZ - 1 &&
					xI != pos.x + dungeonWidthX + 1 && yI != pos.y + dungeonHeight + 1 &&
					zI != pos.z + dungeonWidthZ + 1) {
					world->PlaceBlock(Int3{xI, yI, zI}, BLOCK_AIR);
				} else if (yI >= 0 && !IsSolid(world->GetBlockType(Int3{xI, yI - 1, zI}))) {
					world->PlaceBlock(Int3{xI, yI, zI}, BLOCK_AIR);
				} else if (IsSolid(world->GetBlockType(Int3{xI, yI, zI}))) {
					if (yI == pos.y - 1 && rand.nextInt(4) != 0) {
						world->PlaceBlock(Int3{xI, yI, zI}, BLOCK_COBBLESTONE_MOSSY);
					} else {
						world->PlaceBlock(Int3{xI, yI, zI}, BLOCK_COBBLESTONE);
					}
				}
			}
		}
	}

	std::cout << pos << std::endl;

	// Try placing up to 2 chests
	for (int32_t chestAttempt = 0; chestAttempt < 2; ++chestAttempt) {
		for (int32_t attempts = 0; attempts < 3; ++attempts) {
			int32_t chestX = pos.x + rand.nextInt(dungeonWidthX * 2 + 1) - dungeonWidthX;
			int32_t chestZ = pos.z + rand.nextInt(dungeonWidthZ * 2 + 1) - dungeonWidthZ;

			if (world->GetBlockType(Int3{chestX, pos.y, chestZ}) != BLOCK_AIR)
				continue;

			// Count the number of adjacent blocks
			int32_t adjacentSolidBlocks = 0;
			if (IsSolid(world->GetBlockType(Int3{chestX - 1, pos.y, chestZ})))
				++adjacentSolidBlocks;
			if (IsSolid(world->GetBlockType(Int3{chestX + 1, pos.y, chestZ})))
				++adjacentSolidBlocks;
			if (IsSolid(world->GetBlockType(Int3{chestX, pos.y, chestZ - 1})))
				++adjacentSolidBlocks;
			if (IsSolid(world->GetBlockType(Int3{chestX, pos.y, chestZ + 1})))
				++adjacentSolidBlocks;

			// Only place a block if there's a single solid block
			if (adjacentSolidBlocks == 1) {
				Int3 chestLocation = Int3{chestX, pos.y, chestZ};
				world->PlaceBlock(chestLocation, BLOCK_CHEST);
				std::unique_ptr<ChestTile> chest = std::make_unique<ChestTile>(chestLocation);

				for (int32_t slotAttempt = 0; slotAttempt < 8; ++slotAttempt) {
					Item item = GenerateDungeonChestLoot(rand);
					if (item.id != SLOT_EMPTY) {
						int32_t slot = rand.nextInt(INVENTORY_CHEST_ROWS * INVENTORY_CHEST_COLS);
						chest->inventory.SetSlot(
							Int2{
								slot % INVENTORY_CHEST_COLS,
								slot / INVENTORY_CHEST_ROWS
							}, item
						);
					}
				}
				world->AddTileEntity(std::move(chest));
				break;
			}
		}
	}

	Int3 mobSpawnerPos = Int3{pos.x, pos.y, pos.z};
	world->PlaceBlock(mobSpawnerPos, BLOCK_MOB_SPAWNER);
	world->AddTileEntity(std::make_unique<MobSpawnerTile>(mobSpawnerPos, PickMobToSpawn(rand)));
	return true;
}

/**
 * @brief Generate Dungeon Chest loot
 * 
 * @param rand The random object that should be used for this
 * @return The item that's returned
 */
Item FeatureGenerator::GenerateDungeonChestLoot(Java::Random& rand) {
	int32_t randValue = rand.nextInt(11);
	switch (randValue) {
	case 0:
		return Item{ITEM_SADDLE, 1, 0};
	case 1:
		return Item{ITEM_IRON, int8_t(rand.nextInt(4) + 1), 0};
	case 2:
		return Item{ITEM_BREAD, 1, 0};
	case 3:
		return Item{ITEM_WHEAT, int8_t(rand.nextInt(4) + 1), 0};
	case 4:
		return Item{ITEM_GUNPOWDER, int8_t(rand.nextInt(4) + 1), 0};
	case 5:
		return Item{ITEM_STRING, int8_t(rand.nextInt(4) + 1), 0};
	case 6:
		return Item{ITEM_BUCKET, 1, 0};
	case 7:
		if (rand.nextInt(100) == 0)
			return Item{ITEM_APPLE_GOLDEN, 1, 0};
		break;
	case 8:
		if (rand.nextInt(2) == 0)
			return Item{ITEM_REDSTONE, int8_t(rand.nextInt(4) + 1), 0};
		break;
	case 9:
		if (rand.nextInt(10) == 0)
			return Item{int16_t(ITEM_RECORD_13 + rand.nextInt(2)), 1, 0};
		break;
	case 10:
		return Item{ITEM_DYE, 1, 3};
	}
	return Item{SLOT_EMPTY, 0, 0};
}

/**
 * @brief Pick a random monster that should be in a mob spawner
 * 
 * @param rand The random object that should be used for this
 * @return The name-id of the to be spawned monster
 */
std::string FeatureGenerator::PickMobToSpawn(Java::Random& rand) {
	int32_t mobIndex = rand.nextInt(4);
	switch (mobIndex) {
	case 0:
		return "Skeleton";
	case 1:
	case 2:
		return "Zombie";
	case 3:
		return "Spider";
	}
	return "";
}

/**
 * @brief Generate a clay blob
 * 
 * @param world World in which the feature will attempt to generate
 * @param rand The random object for this feature
 * @param pos Initial position of the feature
 * @return If generation succeeded 
 */
bool FeatureGenerator::GenerateClay(World *world, Java::Random& rand, Int3 pos, int32_t blobSize) {
	// Clay can only generate around water
	BlockType blockType = world->GetBlockType(pos);
	if (blockType != BLOCK_WATER_STILL && blockType != BLOCK_WATER_FLOWING) {
		return false;
	}
	// Get angle of clay blob
	float angle = rand.nextFloat() * float(JavaMath::PI);
	// Then determine the bounds of the blob
	double xStart 	= double(float(pos.x + 8) + MathHelper::sin(angle) * float(blobSize) / 8.0F);
	double xEnd 	= double(float(pos.x + 8) - MathHelper::sin(angle) * float(blobSize) / 8.0F);
	double zStart 	= double(float(pos.z + 8) + MathHelper::cos(angle) * float(blobSize) / 8.0F);
	double zEnd 	= double(float(pos.z + 8) - MathHelper::cos(angle) * float(blobSize) / 8.0F);
	double yStart 	= double(pos.y + rand.nextInt(3) + 2);
	double yEnd 	= double(pos.y + rand.nextInt(3) + 2);

	// Interpolate between the start and end
	for (int32_t i = 0; i <= blobSize; ++i) {
		double xCenter = xStart + (xEnd - xStart) * double(i) / double(blobSize);
		double yCenter = yStart + (yEnd - yStart) * double(i) / double(blobSize);
		double zCenter = zStart + (zEnd - zStart) * double(i) / double(blobSize);
		double blobScale = rand.nextDouble() * double(blobSize) / 16.0;
		double blobRadiusXZ =
			double(MathHelper::sin(float(i) * float(JavaMath::PI) / float(blobSize)) + 1.0F) * blobScale + 1.0;
		double blobRadiusY =
			double(MathHelper::sin(float(i) * float(JavaMath::PI) / float(blobSize)) + 1.0F) * blobScale + 1.0;
		int32_t minX = MathHelper::floor_double(xCenter - blobRadiusXZ / 2.0);
		int32_t maxX = MathHelper::floor_double(xCenter + blobRadiusXZ / 2.0);
		int32_t minY = MathHelper::floor_double(yCenter - blobRadiusY / 2.0);
		int32_t maxY = MathHelper::floor_double(yCenter + blobRadiusY / 2.0);
		int32_t minZ = MathHelper::floor_double(zCenter - blobRadiusXZ / 2.0);
		int32_t maxZ = MathHelper::floor_double(zCenter + blobRadiusXZ / 2.0);

		// Replace sand blocks in the relevant area
		for (int32_t x = minX; x <= maxX; ++x) {
			for (int32_t y = minY; y <= maxY; ++y) {
				for (int32_t z = minZ; z <= maxZ; ++z) {
					double dx = (double(x) + 0.5 - xCenter) / (blobRadiusXZ / 2.0);
					double dy = (double(y) + 0.5 - yCenter) / (blobRadiusY  / 2.0);
					double dz = (double(z) + 0.5 - zCenter) / (blobRadiusXZ / 2.0);
					if (dx * dx + dy * dy + dz * dz < 1.0) {
						BlockType currentBlock = world->GetBlockType(Int3{x, y, z});
						if (currentBlock == BLOCK_SAND) {
							world->PlaceBlock(Int3{x, y, z}, BLOCK_CLAY);
						}
					}
				}
			}
		}
	}

	return true;
}

/**
 * @brief Generate a blob of ore or other material
 * 
 * @param world World in which the feature will attempt to generate
 * @param rand The random object for this feature
 * @param pos Initial position of the feature
 * @return If generation succeeded 
 */
bool FeatureGenerator::GenerateMinable(World *world, Java::Random& rand, Int3 pos, int32_t blobSize) {
	// Get angle of clay blob
	float angle = rand.nextFloat() * float(JavaMath::PI);
	// Then determine the bounds of the blob
	double xStart = double(float(pos.x + 8) + MathHelper::sin(angle) * float(blobSize) / 8.0F);
	double xEnd = double(float(pos.x + 8) - MathHelper::sin(angle) * float(blobSize) / 8.0F);
	double zStart = double(float(pos.z + 8) + MathHelper::cos(angle) * float(blobSize) / 8.0F);
	double zEnd = double(float(pos.z + 8) - MathHelper::cos(angle) * float(blobSize) / 8.0F);
	double yStart = double(pos.y + rand.nextInt(3) + 2);
	double yEnd = double(pos.y + rand.nextInt(3) + 2);

	// Interpolate between the start and end
	for (int32_t i = 0; i <= blobSize; ++i) {
		double xCenter = xStart + (xEnd - xStart) * double(i) / double(blobSize);
		double yCenter = yStart + (yEnd - yStart) * double(i) / double(blobSize);
		double zCenter = zStart + (zEnd - zStart) * double(i) / double(blobSize);
		double blobScale = rand.nextDouble() * double(blobSize) / 16.0;
		double blobRadiusXZ =
			double(MathHelper::sin((float)i * float(JavaMath::PI) / float(blobSize)) + 1.0F) * blobScale + 1.0;
		double blobRadiusY =
			double(MathHelper::sin((float)i * float(JavaMath::PI) / float(blobSize)) + 1.0F) * blobScale + 1.0;
		int32_t minX = MathHelper::floor_double(xCenter - blobRadiusXZ / 2.0);
		int32_t maxX = MathHelper::floor_double(yCenter - blobRadiusY / 2.0);
		int32_t minY = MathHelper::floor_double(zCenter - blobRadiusXZ / 2.0);
		int32_t maxY = MathHelper::floor_double(xCenter + blobRadiusXZ / 2.0);
		int32_t minZ = MathHelper::floor_double(yCenter + blobRadiusY / 2.0);
		int32_t maxZ = MathHelper::floor_double(zCenter + blobRadiusXZ / 2.0);

		// Replace stone blocks in the relevant area
		for (int32_t x = minX; x <= maxY; ++x) {
			double dx = (double(x) + 0.5 - xCenter) / (blobRadiusXZ / 2.0);
			if (dx * dx < 1.0) {
				for (int32_t z = maxX; z <= minZ; ++z) {
					double dz = (double(z) + 0.5 - yCenter) / (blobRadiusY / 2.0);
					if (dx * dx + dz * dz < 1.0) {
						for (int32_t y = minY; y <= maxZ; ++y) {
							double dy = (double(y) + 0.5 - zCenter) / (blobRadiusXZ / 2.0);
							if (dx * dx + dz * dz + dy * dy < 1.0 &&
								world->GetBlockType(Int3{x, z, y}) == BLOCK_STONE) {
								world->SetBlockType(BlockType(this->type), Int3{x, z, y});
							}
						}
					}
				}
			}
		}
	}

	return true;
}

/**
 * @brief Used to generate flowers and mushrooms
 * 
 * @param world World in which the feature will attempt to generate
 * @param rand The random object for this feature
 * @param pos Initial position of the feature
 * @return If generation succeeded 
 */
bool FeatureGenerator::GenerateFlowers(World *world, Java::Random& rand, Int3 pos) {
	for (int32_t i = 0; i < CHUNK_HEIGHT / 2; ++i) {
		int32_t offsetX = pos.x + rand.nextInt(8) - rand.nextInt(8);
		int32_t offsetY = pos.y + rand.nextInt(4) - rand.nextInt(4);
		int32_t offsetZ = pos.z + rand.nextInt(8) - rand.nextInt(8);
		if (world->GetBlockType(Int3{offsetX, offsetY, offsetZ}) == BLOCK_AIR &&
			CanStay(this->type, world, Int3{offsetX, offsetY, offsetZ})) {
			world->SetBlockType(this->type, Int3{offsetX, offsetY, offsetZ});
		}
	}

	return true;
}

/**
 * @brief Generate tallgrass and ferns
 * 
 * @param world World in which the feature will attempt to generate
 * @param rand The random object for this feature
 * @param pos Initial position of the feature
 * @return If generation succeeded 
 */
bool FeatureGenerator::GenerateTallgrass(World *world, Java::Random& rand, Int3 pos) {
	while (true) {
		BlockType blockType = world->GetBlockType(Int3{pos.x, pos.y, pos.z});
		if ((blockType != 0 && blockType != BLOCK_LEAVES) || pos.y <= 0) {
			for (int32_t y = 0; y < CHUNK_HEIGHT; ++y) {
				int32_t offsetX = pos.x + rand.nextInt(8) - rand.nextInt(8);
				int32_t offsetY = pos.y + rand.nextInt(4) - rand.nextInt(4);
				int32_t offsetZ = pos.z + rand.nextInt(8) - rand.nextInt(8);
				if (world->GetBlockType(Int3{offsetX, offsetY, offsetZ}) == BLOCK_AIR &&
					CanStay(this->type, world, Int3{offsetX, offsetY, offsetZ})) {
					world->SetBlockTypeAndMeta(this->type, this->meta, Int3{offsetX, offsetY, offsetZ});
				}
			}

			return true;
		}

		--pos.y;
	}
}

/**
 * @brief Generate deadbushes
 * 
 * @param world World in which the feature will attempt to generate
 * @param rand The random object for this feature
 * @param pos Initial position of the feature
 * @return If generation succeeded 
 */
bool FeatureGenerator::GenerateDeadbush(World *world, Java::Random& rand, Int3 pos) {
	while (true) {
		BlockType blockType = world->GetBlockType(Int3{pos.x, pos.y, pos.z});
		if ((blockType != 0 && blockType != BLOCK_LEAVES) || pos.y <= 0) {
			for (int32_t i = 0; i < 4; ++i) {
				int32_t offsetX = pos.x + rand.nextInt(8) - rand.nextInt(8);
				int32_t offsetY = pos.y + rand.nextInt(4) - rand.nextInt(4);
				int32_t offsetZ = pos.z + rand.nextInt(8) - rand.nextInt(8);
				if (world->GetBlockType(Int3{offsetX, offsetY, offsetZ}) == BLOCK_AIR &&
					CanStay(this->type, world, Int3{offsetX, offsetY, offsetZ})) {
					world->SetBlockType(this->type, Int3{offsetX, offsetY, offsetZ});
				}
			}

			return true;
		}

		--pos.y;
	}
}

/**
 * @brief Generate sugarcane
 * 
 * @param world World in which the feature will attempt to generate
 * @param rand The random object for this feature
 * @param pos Initial position of the feature
 * @return If generation succeeded 
 */
bool FeatureGenerator::GenerateSugarcane(World *world, Java::Random& rand, Int3 pos) {
	for (int32_t i = 0; i < 20; ++i) {
		int32_t xOffset = pos.x + rand.nextInt(4) - rand.nextInt(4);
		int32_t zOffset = pos.z + rand.nextInt(4) - rand.nextInt(4);
		if (world->GetBlockType(Int3{xOffset, pos.y, zOffset}) == BLOCK_AIR && 
			(world->GetBlockType(Int3{xOffset - 1, pos.y - 1, zOffset}) == BLOCK_WATER_STILL ||
			 world->GetBlockType(Int3{xOffset - 1, pos.y - 1, zOffset}) == BLOCK_WATER_FLOWING ||
			 world->GetBlockType(Int3{xOffset + 1, pos.y - 1, zOffset}) == BLOCK_WATER_STILL ||
			 world->GetBlockType(Int3{xOffset + 1, pos.y - 1, zOffset}) == BLOCK_WATER_FLOWING ||
			 world->GetBlockType(Int3{xOffset, pos.y - 1, zOffset - 1}) == BLOCK_WATER_STILL ||
			 world->GetBlockType(Int3{xOffset, pos.y - 1, zOffset - 1}) == BLOCK_WATER_FLOWING ||
			 world->GetBlockType(Int3{xOffset, pos.y - 1, zOffset + 1}) == BLOCK_WATER_STILL ||
			 world->GetBlockType(Int3{xOffset, pos.y - 1, zOffset + 1}) == BLOCK_WATER_FLOWING)) {
			int32_t height = 2 + rand.nextInt(rand.nextInt(3) + 1);

			for (int32_t h = 0; h < height; ++h) {
				if (CanStay(BLOCK_SUGARCANE, world, Int3{xOffset, pos.y + h, zOffset})) {
					world->SetBlockType(BLOCK_SUGARCANE, Int3{xOffset, pos.y + h, zOffset});
				}
			}
		}
	}

	return true;
}

/**
 * @brief Generate pumpkins
 * 
 * @param world World in which the feature will attempt to generate
 * @param rand The random object for this feature
 * @param pos Initial position of the feature
 * @return If generation succeeded 
 */
bool FeatureGenerator::GeneratePumpkins(World *world, Java::Random& rand, Int3 pos) {
	for (int32_t i = 0; i < 64; ++i) {
		int32_t xOffset = pos.x + rand.nextInt(8) - rand.nextInt(8);
		int32_t yOffset = pos.y + rand.nextInt(4) - rand.nextInt(4);
		int32_t zOffset = pos.z + rand.nextInt(8) - rand.nextInt(8);
		if (world->GetBlockType(Int3{xOffset, yOffset, zOffset}) == BLOCK_AIR &&
			world->GetBlockType(Int3{xOffset, yOffset - 1, zOffset}) == BLOCK_GRASS &&
			CanBePlaced(BLOCK_PUMPKIN, world, Int3{xOffset, yOffset, zOffset})) {
			world->SetBlockTypeAndMeta(BLOCK_PUMPKIN, rand.nextInt(4), Int3{xOffset, yOffset, zOffset});
		}
	}
	return true;
}

/**
 * @brief Generate cacti
 * 
 * @param world World in which the feature will attempt to generate
 * @param rand The random object for this feature
 * @param pos Initial position of the feature
 * @return If generation succeeded 
 */
bool FeatureGenerator::GenerateCacti(World *world, Java::Random& rand, Int3 pos) {
	for (int32_t i = 0; i < 10; ++i) {
		int32_t xOffset = pos.x + rand.nextInt(8) - rand.nextInt(8);
		int32_t yOffset = pos.y + rand.nextInt(4) - rand.nextInt(4);
		int32_t zOffset = pos.z + rand.nextInt(8) - rand.nextInt(8);
		if (world->GetBlockType(Int3{xOffset, yOffset, zOffset}) == BLOCK_AIR) {
			int32_t height = 1 + rand.nextInt(rand.nextInt(3) + 1);

			for (int32_t h = 0; h < height; ++h) {
				if (CanStay(BLOCK_CACTUS, world, Int3{xOffset, yOffset + h, zOffset})) {
					world->SetBlockType(BLOCK_CACTUS, Int3{xOffset, yOffset + h, zOffset});
				}
			}
		}
	}

	return true;
}

/**
 * @brief Generate single-block liquid sources
 * 
 * @param world World in which the feature will attempt to generate
 * @param rand The random object for this feature
 * @param pos Initial position of the feature
 * @return If generation succeeded 
 */
bool FeatureGenerator::GenerateLiquid(World *world, [[maybe_unused]] Java::Random& rand, Int3 pos) {
	if (world->GetBlockType(Int3{pos.x, pos.y + 1, pos.z}) != BLOCK_STONE) {
		return false;
	} else if (world->GetBlockType(Int3{pos.x, pos.y - 1, pos.z}) != BLOCK_STONE) {
		return false;
	} else if (world->GetBlockType(Int3{pos.x, pos.y, pos.z}) != BLOCK_AIR &&
			   world->GetBlockType(Int3{pos.x, pos.y, pos.z}) != BLOCK_STONE) {
		return false;
	} else {
		// Check for surrounding stone along non-vertical directions
		int32_t surroundingStone = 0;
		if (world->GetBlockType(Int3{pos.x - 1, pos.y, pos.z}) == BLOCK_STONE)
			++surroundingStone;
		if (world->GetBlockType(Int3{pos.x + 1, pos.y, pos.z}) == BLOCK_STONE)
			++surroundingStone;
		if (world->GetBlockType(Int3{pos.x, pos.y, pos.z - 1}) == BLOCK_STONE)
			++surroundingStone;
		if (world->GetBlockType(Int3{pos.x, pos.y, pos.z + 1}) == BLOCK_STONE)
			++surroundingStone;

		// Check for surrounding air along non-vertical directions
		int32_t surroundingAir = 0;
		if (world->GetBlockType(Int3{pos.x - 1, pos.y, pos.z}) == BLOCK_AIR)
			++surroundingAir;
		if (world->GetBlockType(Int3{pos.x + 1, pos.y, pos.z}) == BLOCK_AIR)
			++surroundingAir;
		if (world->GetBlockType(Int3{pos.x, pos.y, pos.z - 1}) == BLOCK_AIR)
			++surroundingAir;
		if (world->GetBlockType(Int3{pos.x, pos.y, pos.z + 1}) == BLOCK_AIR)
			++surroundingAir;

		// If there's exactly 3 stone and 1 air, place the liquid block
		if (surroundingStone == 3 && surroundingAir == 1) {
			world->SetBlockType(this->type, Int3{pos.x, pos.y, pos.z});
			// var1.scheduledUpdatesAreImmediate = true;
			// Block.blocksList[this.liquidBlockId].updateTick(var1, var3, var4, var5, var2);
			// var1.scheduledUpdatesAreImmediate = false;
		}
		return true;
	}
}