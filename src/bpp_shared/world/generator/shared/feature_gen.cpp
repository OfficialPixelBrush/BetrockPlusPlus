/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "feature_gen.h"
#include "tile_entities/tile_entity.h"

//  GenerateLake
bool FeatureGenerator::GenerateLake(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	_pos.x -= 8;
	_pos.z -= 8;

	// Sink to first non-air block
	while (_pos.y > 0 && _world.getBlockId({ _pos.x, _pos.y, _pos.z }) == BLOCK_AIR)
		--_pos.y;

	_pos.y -= 4;

	bool shapeMask[2048] = {};
	int32_t blobCount = _rand.nextInt(4) + 4;

	for (int32_t blobIndex = 0; blobIndex < blobCount; ++blobIndex) {
		double radX = _rand.nextDouble() * 6.0 + 3.0;
		double radY = _rand.nextDouble() * 4.0 + 2.0;
		double radZ = _rand.nextDouble() * 6.0 + 3.0;
		double cx = _rand.nextDouble() * (16.0 - radX - 2.0) + 1.0 + radX / 2.0;
		double cy = _rand.nextDouble() * (8.0 - radY - 4.0) + 2.0 + radY / 2.0;
		double cz = _rand.nextDouble() * (16.0 - radZ - 2.0) + 1.0 + radZ / 2.0;

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
				bool edge = !shapeMask[(x * 16 + z) * 8 + y] && ((x < 15 && shapeMask[((x + 1) * 16 + z) * 8 + y]) ||
				                                                 (x > 0 && shapeMask[((x - 1) * 16 + z) * 8 + y]) ||
				                                                 (z < 15 && shapeMask[(x * 16 + z + 1) * 8 + y]) ||
				                                                 (z > 0 && shapeMask[(x * 16 + z - 1) * 8 + y]) ||
				                                                 (y < 7 && shapeMask[(x * 16 + z) * 8 + y + 1]) ||
				                                                 (y > 0 && shapeMask[(x * 16 + z) * 8 + y - 1]));
				if (!edge)
					continue;
				BlockType bt = _world.getBlockId({ _pos.x + x, _pos.y + y, _pos.z + z });
				if (y >= 4 && IsLiquid(bt))
					return false;
				if (y < 4 && !IsSolid(bt) && bt != this->type)
					return false;
			}

	// Fill
	for (int32_t x = 0; x < 16; ++x)
		for (int32_t z = 0; z < 16; ++z)
			for (int32_t y = 0; y < 8; ++y)
				if (shapeMask[(x * 16 + z) * 8 + y])
					_world.setBlock({ _pos.x + x, _pos.y + y, _pos.z + z }, y >= 4 ? BLOCK_AIR : this->type);

	// Exposed dirt -> grass
	for (int32_t x = 0; x < 16; ++x)
		for (int32_t z = 0; z < 16; ++z)
			for (int32_t y = 4; y < 8; ++y)
				if (shapeMask[(x * 16 + z) * 8 + y] &&
				    _world.getBlockId({ _pos.x + x, _pos.y + y - 1, _pos.z + z }) == BLOCK_DIRT &&
				    _world.getSkyLight({ _pos.x + x, _pos.y + y, _pos.z + z }) > 0)
					_world.setBlock({ _pos.x + x, _pos.y + y - 1, _pos.z + z }, BLOCK_GRASS);

	// Lava: solidify exposed edges
	if (this->type == BLOCK_LAVA_STILL || this->type == BLOCK_LAVA_FLOWING) {
		for (int32_t x = 0; x < 16; ++x)
			for (int32_t z = 0; z < 16; ++z)
				for (int32_t y = 0; y < 8; ++y) {
					bool edge = !shapeMask[(x * 16 + z) * 8 + y] && ((x < 15 && shapeMask[((x + 1) * 16 + z) * 8 + y]) ||
					                                                 (x > 0 && shapeMask[((x - 1) * 16 + z) * 8 + y]) ||
					                                                 (z < 15 && shapeMask[(x * 16 + z + 1) * 8 + y]) ||
					                                                 (z > 0 && shapeMask[(x * 16 + z - 1) * 8 + y]) ||
					                                                 (y < 7 && shapeMask[(x * 16 + z) * 8 + y + 1]) ||
					                                                 (y > 0 && shapeMask[(x * 16 + z) * 8 + y - 1]));
					if (edge && (y < 4 || _rand.nextInt(2) != 0) &&
					    IsSolid(_world.getBlockId({ _pos.x + x, _pos.y + y, _pos.z + z })))
						_world.setBlock({ _pos.x + x, _pos.y + y, _pos.z + z }, BLOCK_STONE);
				}
	}
	return true;
}

//  GenerateDungeon
bool FeatureGenerator::GenerateDungeon(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	const int8_t dungeonHeight = 3;
	int32_t dungeonWidthX = _rand.nextInt(2) + 2;
	int32_t dungeonWidthZ = _rand.nextInt(2) + 2;
	int32_t validEntries = 0;

	for (int32_t xi = _pos.x - dungeonWidthX - 1; xi <= _pos.x + dungeonWidthX + 1; ++xi)
		for (int32_t yi = _pos.y - 1; yi <= _pos.y + dungeonHeight + 1; ++yi)
			for (int32_t zi = _pos.z - dungeonWidthZ - 1; zi <= _pos.z + dungeonWidthZ + 1; ++zi) {
				BlockType bt = _world.getBlockId({ xi, yi, zi });
				if (yi == _pos.y - 1 && !IsSolid(bt))
					return false;
				if (yi == _pos.y + dungeonHeight + 1 && !IsSolid(bt))
					return false;
				bool isWall = (xi == _pos.x - dungeonWidthX - 1 || xi == _pos.x + dungeonWidthX + 1 ||
				               zi == _pos.z - dungeonWidthZ - 1 || zi == _pos.z + dungeonWidthZ + 1);
				if (isWall && yi == _pos.y && bt == BLOCK_AIR && _world.getBlockId({ xi, yi + 1, zi }) == BLOCK_AIR)
					++validEntries;
			}

	if (validEntries < 1 || validEntries > 5)
		return false;

	for (int32_t xi = _pos.x - dungeonWidthX - 1; xi <= _pos.x + dungeonWidthX + 1; ++xi)
		for (int32_t yi = _pos.y + dungeonHeight; yi >= _pos.y - 1; --yi)
			for (int32_t zi = _pos.z - dungeonWidthZ - 1; zi <= _pos.z + dungeonWidthZ + 1; ++zi) {
				bool interior = (xi != _pos.x - dungeonWidthX - 1 && xi != _pos.x + dungeonWidthX + 1 &&
				                 yi != _pos.y - 1 && yi != _pos.y + dungeonHeight + 1 &&
				                 zi != _pos.z - dungeonWidthZ - 1 && zi != _pos.z + dungeonWidthZ + 1);
				if (interior) {
					_world.setBlock({ xi, yi, zi }, BLOCK_AIR);
				} else if (yi >= 0 && !IsSolid(_world.getBlockId({ xi, yi - 1, zi }))) {
					_world.setBlock({ xi, yi, zi }, BLOCK_AIR);
				} else if (IsSolid(_world.getBlockId({ xi, yi, zi }))) {
					BlockType wall = (yi == _pos.y - 1 && _rand.nextInt(4) != 0) ? BLOCK_COBBLESTONE_MOSSY
					                                                           : BLOCK_COBBLESTONE;
					_world.setBlock({ xi, yi, zi }, wall);
				}
			}

	// Up to 2 chests, 3 placement attempts each
	for (int32_t chestAttempt = 0; chestAttempt < 2; ++chestAttempt) {
		for (int32_t attempt = 0; attempt < 3; ++attempt) {
			int32_t cx = _pos.x + _rand.nextInt(dungeonWidthX * 2 + 1) - dungeonWidthX;
			int32_t cz = _pos.z + _rand.nextInt(dungeonWidthZ * 2 + 1) - dungeonWidthZ;
			if (_world.getBlockId({ cx, _pos.y, cz }) != BLOCK_AIR)
				continue;
			int32_t adj = 0;
			if (IsSolid(_world.getBlockId({ cx - 1, _pos.y, cz })))
				++adj;
			if (IsSolid(_world.getBlockId({ cx + 1, _pos.y, cz })))
				++adj;
			if (IsSolid(_world.getBlockId({ cx, _pos.y, cz - 1 })))
				++adj;
			if (IsSolid(_world.getBlockId({ cx, _pos.y, cz + 1 })))
				++adj;
			if (adj == 1) {
				_world.setBlock({ cx, _pos.y, cz }, BLOCK_CHEST);
				auto chest = std::make_shared<TileEntityChest>(Int3{ cx, _pos.y, cz });
				for (int32_t slot = 0; slot < 8; ++slot) {
					auto stack = GenerateDungeonChestLoot(_rand);
					if (stack.id != Items::Id::INVALID) {
						int32_t slotIndex = _rand.nextInt(27);
						chest->inventory.setInventorySlotContents(slotIndex, &stack);
					}
				}
				_world.manager.createTileEntity(std::move(chest));
				break;
			}
		}
	}

	_world.setBlock(_pos, BLOCK_MOB_SPAWNER);
	auto spawner = std::make_shared<TileEntityMobSpawner>(_pos);
	spawner->entityId = PickMobToSpawn(_rand);
	_world.manager.createTileEntity(std::move(spawner));
	return true;
}

// Creates Dungeon Chest loot
ItemStack FeatureGenerator::GenerateDungeonChestLoot(Java::Random& _rand) {
	int32_t roll = _rand.nextInt(11);
	switch (roll) {
	case 0:
		return { .id = Items::Id::SADDLE, .count = 1 };
	case 1: {
		int8_t qty = _rand.nextInt(4) + 1;
		return { .id = Items::Id::IRON, .count = qty };
	}
	case 2:
		return { .id = Items::Id::BREAD, .count = 1 };
	case 3: {
		int8_t qty = _rand.nextInt(4) + 1;
		return { .id = Items::Id::WHEAT, .count = qty };
	}
	case 4: {
		int8_t qty = _rand.nextInt(4) + 1;
		return { .id = Items::Id::GUNPOWDER, .count = qty };
	}
	case 5: {
		int8_t qty = _rand.nextInt(4) + 1;
		return { .id = Items::Id::STRING, .count = qty };
	}
	case 6:
		return { .id = Items::Id::BUCKET, .count = 1 };
	case 7:
		if (_rand.nextInt(100) == 0)
			return { .id = Items::Id::APPLE_GOLDEN, .count = 1 };
		return { .id = Items::Id::INVALID };
	case 8:
		if (_rand.nextInt(2) == 0) {
			int8_t qty = _rand.nextInt(4) + 1;
			return { .id = Items::Id::REDSTONE, .count = qty };
		}
		return { .id = Items::Id::INVALID };
	case 9:
		if (_rand.nextInt(10) == 0) {
			int16_t discId = (_rand.nextInt(2) == 0) ? Items::Id::RECORD_13 : Items::Id::RECORD_CAT;
			return { .id = discId, .count = 1 };
		}
		return { .id = Items::Id::INVALID };
	case 10:
		return { .id = Items::Id::DYE, .count = 1, .data = 3 };
	default:
		return { .id = Items::Id::INVALID };
	}
}

std::string FeatureGenerator::PickMobToSpawn(Java::Random& _rand) {
	switch (_rand.nextInt(4)) {
	case 0:
		return "Skeleton";
	case 1:
	case 2:
		return "Zombie";
	case 3:
		return "Spider";
	default:
		return "Zombie";
	}
}

//  GenerateClay
bool FeatureGenerator::GenerateClay(WorldWrapper& _world, Java::Random& _rand, Int3 _pos, int32_t _blobSize) {
	BlockType at = _world.getBlockId(_pos);
	if (at != BLOCK_WATER_STILL && at != BLOCK_WATER_FLOWING)
		return false;

	float angle = _rand.nextFloat() * JavaMath::PI_FLOAT;
	double xStart = double(float(_pos.x + 8) + MathHelper::sin(angle) * float(_blobSize) / 8.0F);
	double xEnd = double(float(_pos.x + 8) - MathHelper::sin(angle) * float(_blobSize) / 8.0F);
	double zStart = double(float(_pos.z + 8) + MathHelper::cos(angle) * float(_blobSize) / 8.0F);
	double zEnd = double(float(_pos.z + 8) - MathHelper::cos(angle) * float(_blobSize) / 8.0F);
	double yStart = double(_pos.y + _rand.nextInt(3) + 2);
	double yEnd = double(_pos.y + _rand.nextInt(3) + 2);

	for (int32_t i = 0; i <= _blobSize; ++i) {
		double xC = xStart + (xEnd - xStart) * double(i) / double(_blobSize);
		double yC = yStart + (yEnd - yStart) * double(i) / double(_blobSize);
		double zC = zStart + (zEnd - zStart) * double(i) / double(_blobSize);
		double blobScale = _rand.nextDouble() * double(_blobSize) / 16.0;
		double radXZ = double(MathHelper::sin(float(i) * JavaMath::PI_FLOAT / float(_blobSize)) + 1.0F) * blobScale + 1.0;
		double radY = double(MathHelper::sin(float(i) * JavaMath::PI_FLOAT / float(_blobSize)) + 1.0F) * blobScale + 1.0;
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
					if (dx * dx + dy * dy + dz * dz < 1.0 && _world.getBlockId({ x, y, z }) == BLOCK_SAND)
						_world.setBlock({ x, y, z }, BLOCK_CLAY);
				}
	}
	return true;
}

//  GenerateMinable
bool FeatureGenerator::GenerateMinable(WorldWrapper& _world, Java::Random& _rand, Int3 _pos, int32_t _blobSize) {
	float angle = _rand.nextFloat() * JavaMath::PI_FLOAT;
	double xStart = double(float(_pos.x + 8) + MathHelper::sin(angle) * float(_blobSize) / 8.0F);
	double xEnd = double(float(_pos.x + 8) - MathHelper::sin(angle) * float(_blobSize) / 8.0F);
	double zStart = double(float(_pos.z + 8) + MathHelper::cos(angle) * float(_blobSize) / 8.0F);
	double zEnd = double(float(_pos.z + 8) - MathHelper::cos(angle) * float(_blobSize) / 8.0F);
	double yStart = double(_pos.y + _rand.nextInt(3) + 2);
	double yEnd = double(_pos.y + _rand.nextInt(3) + 2);

	for (int32_t i = 0; i <= _blobSize; ++i) {
		double xC = xStart + (xEnd - xStart) * double(i) / double(_blobSize);
		double yC = yStart + (yEnd - yStart) * double(i) / double(_blobSize);
		double zC = zStart + (zEnd - zStart) * double(i) / double(_blobSize);
		double blobScale = _rand.nextDouble() * double(_blobSize) / 16.0;
		double radXZ = double(MathHelper::sin(float(i) * JavaMath::PI_FLOAT / float(_blobSize)) + 1.0F) * blobScale + 1.0;
		double radY = double(MathHelper::sin(float(i) * JavaMath::PI_FLOAT / float(_blobSize)) + 1.0F) * blobScale + 1.0;
		int32_t minX = MathHelper::floor_double(xC - radXZ / 2.0);
		int32_t maxX = MathHelper::floor_double(xC + radXZ / 2.0);
		int32_t minY = MathHelper::floor_double(yC - radY / 2.0);
		int32_t maxY = MathHelper::floor_double(yC + radY / 2.0);
		int32_t minZ = MathHelper::floor_double(zC - radXZ / 2.0);
		int32_t maxZ = MathHelper::floor_double(zC + radXZ / 2.0);
		for (int32_t x = minX; x <= maxX; ++x) {
			double dx = (double(x) + 0.5 - xC) / (radXZ / 2.0);
			if (dx * dx >= 1.0)
				continue;
			for (int32_t y = minY; y <= maxY; ++y) {
				double dy = (double(y) + 0.5 - yC) / (radY / 2.0);
				if (dx * dx + dy * dy >= 1.0)
					continue;
				for (int32_t z = minZ; z <= maxZ; ++z) {
					double dz = (double(z) + 0.5 - zC) / (radXZ / 2.0);
					if (dx * dx + dy * dy + dz * dz < 1.0 && _world.getBlockId({ x, y, z }) == BLOCK_STONE)
						_world.setBlock({ x, y, z }, this->type);
				}
			}
		}
	}
	return true;
}

//  Attempts to generate flower/mushroom patches
bool FeatureGenerator::GenerateFlowers(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	bool isMushroom = (this->type == BLOCK_MUSHROOM_BROWN || this->type == BLOCK_MUSHROOM_RED);

	for (int32_t i = 0; i < 64; ++i) {
		int32_t x = _pos.x + _rand.nextInt(8) - _rand.nextInt(8);
		int32_t y = _pos.y + _rand.nextInt(4) - _rand.nextInt(4);
		int32_t z = _pos.z + _rand.nextInt(8) - _rand.nextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT)
			continue;
		if (_world.getBlockId({ x, y, z }) != BLOCK_AIR)
			continue;

		if (isMushroom) {
			if (IsSolid(_world.getBlockId({ x, y - 1, z })) && _world.getSkyLight({ x, y, z }) == 0)
				_world.setBlock({ x, y, z }, this->type);
		} else {
			BlockType below = _world.getBlockId({ x, y - 1, z });
			if (below == BLOCK_GRASS)
				_world.setBlock({ x, y, z }, this->type);
		}
	}
	return true;
}

//  Attempts to generate tallgrass patches
bool FeatureGenerator::GenerateTallgrass(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	while (_pos.y > 0) {
		BlockType b = _world.getBlockId({ _pos.x, _pos.y, _pos.z });
		if (b != BLOCK_AIR && b != BLOCK_LEAVES)
			break;
		--_pos.y;
	}

	for (int32_t i = 0; i < 128; ++i) {
		int32_t x = _pos.x + _rand.nextInt(8) - _rand.nextInt(8);
		int32_t y = _pos.y + _rand.nextInt(4) - _rand.nextInt(4);
		int32_t z = _pos.z + _rand.nextInt(8) - _rand.nextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT)
			continue;
		if (_world.getBlockId({ x, y, z }) != BLOCK_AIR)
			continue;
		BlockType below = _world.getBlockId({ x, y - 1, z });
		if (below == BLOCK_GRASS || below == BLOCK_DIRT)
			_world.setBlock({ x, y, z }, this->type, uint8_t(this->meta));
	}
	return true;
}

//  Attempts to generate deadbush patches
bool FeatureGenerator::GenerateDeadbush(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	while (_pos.y > 0) {
		BlockType b = _world.getBlockId({ _pos.x, _pos.y, _pos.z });
		if (b != BLOCK_AIR && b != BLOCK_LEAVES)
			break;
		--_pos.y;
	}

	for (int32_t i = 0; i < 4; ++i) {
		int32_t x = _pos.x + _rand.nextInt(8) - _rand.nextInt(8);
		int32_t y = _pos.y + _rand.nextInt(4) - _rand.nextInt(4);
		int32_t z = _pos.z + _rand.nextInt(8) - _rand.nextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT)
			continue;
		if (_world.getBlockId({ x, y, z }) == BLOCK_AIR && _world.getBlockId({ x, y - 1, z }) == BLOCK_SAND)
			_world.setBlock({ x, y, z }, BLOCK_DEADBUSH);
	}
	return true;
}

//  Attempts to generate sugarcane patches
bool FeatureGenerator::GenerateSugarcane(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	for (int32_t i = 0; i < 20; ++i) {
		int32_t x = _pos.x + _rand.nextInt(4) - _rand.nextInt(4);
		int32_t y = _pos.y; // Y is fixed across all attempts
		int32_t z = _pos.z + _rand.nextInt(4) - _rand.nextInt(4);
		if (_world.getBlockId({ x, y, z }) != BLOCK_AIR)
			continue;

		auto isWater = [&](int _wx, int _wy, int _wz) {
			BlockType b = _world.getBlockId({ _wx, _wy, _wz });
			return b == BLOCK_WATER_STILL || b == BLOCK_WATER_FLOWING;
		};
		if (!isWater(x - 1, y - 1, z) && !isWater(x + 1, y - 1, z) && !isWater(x, y - 1, z - 1) &&
		    !isWater(x, y - 1, z + 1))
			continue;

		int32_t height = 2 + _rand.nextInt(_rand.nextInt(3) + 1);
		for (int32_t h = 0; h < height; ++h) {
			BlockType below = _world.getBlockId({ x, y + h - 1, z });
			if (below != BLOCK_GRASS && below != BLOCK_DIRT && below != BLOCK_SUGARCANE)
				break;
			if (_world.getBlockId({ x, y + h, z }) != BLOCK_AIR)
				break;
			_world.setBlock({ x, y + h, z }, BLOCK_SUGARCANE);
		}
	}
	return true;
}

//  Attempts to generate pumpkin patches
bool FeatureGenerator::GeneratePumpkins(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	for (int32_t i = 0; i < 64; ++i) {
		int32_t x = _pos.x + _rand.nextInt(8) - _rand.nextInt(8);
		int32_t y = _pos.y + _rand.nextInt(4) - _rand.nextInt(4);
		int32_t z = _pos.z + _rand.nextInt(8) - _rand.nextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT)
			continue;
		if (_world.getBlockId({ x, y, z }) != BLOCK_AIR)
			continue;
		if (_world.getBlockId({ x, y - 1, z }) != BLOCK_GRASS)
			continue;
		// canPlaceBlockAt: no adjacent pumpkins on cardinal sides
		if (_world.getBlockId({ x - 1, y, z }) == BLOCK_PUMPKIN)
			continue;
		if (_world.getBlockId({ x + 1, y, z }) == BLOCK_PUMPKIN)
			continue;
		if (_world.getBlockId({ x, y, z - 1 }) == BLOCK_PUMPKIN)
			continue;
		if (_world.getBlockId({ x, y, z + 1 }) == BLOCK_PUMPKIN)
			continue;
		_world.setBlock({ x, y, z }, BLOCK_PUMPKIN, uint8_t(_rand.nextInt(4)));
	}
	return true;
}

//  Attempts to generate cacti patches
bool FeatureGenerator::GenerateCacti(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	for (int32_t i = 0; i < 10; ++i) {
		int32_t x = _pos.x + _rand.nextInt(8) - _rand.nextInt(8);
		int32_t y = _pos.y + _rand.nextInt(4) - _rand.nextInt(4);
		int32_t z = _pos.z + _rand.nextInt(8) - _rand.nextInt(8);
		if (_world.getBlockId({ x, y, z }) != BLOCK_AIR)
			continue;

		int32_t height = 1 + _rand.nextInt(_rand.nextInt(3) + 1);
		for (int32_t h = 0; h < height; ++h) {
			if (Blocks::blockProperties[_world.getBlockId({ x - 1, y + h, z })].material.isSolid)
				continue;
			if (Blocks::blockProperties[_world.getBlockId({ x + 1, y + h, z })].material.isSolid)
				continue;
			if (Blocks::blockProperties[_world.getBlockId({ x, y + h, z - 1 })].material.isSolid)
				continue;
			if (Blocks::blockProperties[_world.getBlockId({ x, y + h, z + 1 })].material.isSolid)
				continue;
			BlockType below = _world.getBlockId({ x, y + h - 1, z });
			if (below == BLOCK_SAND || below == BLOCK_CACTUS)
				_world.setBlock({ x, y + h, z }, BLOCK_CACTUS);
		}
	}
	return true;
}

//  Attempts to generate a singular liquid source block
bool FeatureGenerator::GenerateLiquid(WorldWrapper& _world, [[maybe_unused]] Java::Random& _rand, Int3 _pos) {
	if (_world.getBlockId({ _pos.x, _pos.y + 1, _pos.z }) != BLOCK_STONE)
		return false;
	if (_world.getBlockId({ _pos.x, _pos.y - 1, _pos.z }) != BLOCK_STONE)
		return false;
	BlockType cur = _world.getBlockId(_pos);
	if (cur != BLOCK_AIR && cur != BLOCK_STONE)
		return false;

	int32_t stone = 0, air = 0;
	if (_world.getBlockId({ _pos.x - 1, _pos.y, _pos.z }) == BLOCK_STONE)
		++stone;
	if (_world.getBlockId({ _pos.x + 1, _pos.y, _pos.z }) == BLOCK_STONE)
		++stone;
	if (_world.getBlockId({ _pos.x, _pos.y, _pos.z - 1 }) == BLOCK_STONE)
		++stone;
	if (_world.getBlockId({ _pos.x, _pos.y, _pos.z + 1 }) == BLOCK_STONE)
		++stone;
	if (_world.getBlockId({ _pos.x - 1, _pos.y, _pos.z }) == BLOCK_AIR)
		++air;
	if (_world.getBlockId({ _pos.x + 1, _pos.y, _pos.z }) == BLOCK_AIR)
		++air;
	if (_world.getBlockId({ _pos.x, _pos.y, _pos.z - 1 }) == BLOCK_AIR)
		++air;
	if (_world.getBlockId({ _pos.x, _pos.y, _pos.z + 1 }) == BLOCK_AIR)
		++air;

	if (stone == 3 && air == 1) {
		_world.setBlock(_pos, this->type);
	}
	return true;
}

// Nether Features

// TODO: Merge with GenerateLiquid?
//  GenerateNetherLiquid
bool FeatureGenerator::GenerateNetherLiquid(WorldWrapper& _world, [[maybe_unused]] Java::Random& _rand, Int3 _pos) {
	if (_world.getBlockId({ _pos.x, _pos.y + 1, _pos.z }) != BLOCK_NETHERRACK)
		return false;
	if (_world.getBlockId({ _pos.x, _pos.y - 1, _pos.z }) != BLOCK_NETHERRACK)
		return false;
	BlockType cur = _world.getBlockId(_pos);
	if (cur != BLOCK_AIR && cur != BLOCK_NETHERRACK)
		return false;

	int32_t netherrack = 0, air = 0;
	if (_world.getBlockId({ _pos.x - 1, _pos.y, _pos.z }) == BLOCK_NETHERRACK)
		++netherrack;
	if (_world.getBlockId({ _pos.x + 1, _pos.y, _pos.z }) == BLOCK_NETHERRACK)
		++netherrack;
	if (_world.getBlockId({ _pos.x, _pos.y, _pos.z - 1 }) == BLOCK_NETHERRACK)
		++netherrack;
	if (_world.getBlockId({ _pos.x, _pos.y, _pos.z + 1 }) == BLOCK_NETHERRACK)
		++netherrack;
	if (_world.getBlockId({ _pos.x - 1, _pos.y, _pos.z }) == BLOCK_AIR)
		++air;
	if (_world.getBlockId({ _pos.x + 1, _pos.y, _pos.z }) == BLOCK_AIR)
		++air;
	if (_world.getBlockId({ _pos.x, _pos.y, _pos.z - 1 }) == BLOCK_AIR)
		++air;
	if (_world.getBlockId({ _pos.x, _pos.y, _pos.z + 1 }) == BLOCK_AIR)
		++air;

	if (netherrack == 3 && air == 1) {
		_world.setBlock(_pos, this->type);
	}
	return true;
}

//  GenerateNetherFire
bool FeatureGenerator::GenerateNetherFire(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	for (int i = 0; i < 64; ++i) {
		Int3 test_pos{
			_pos.x + _rand.nextInt(8) - _rand.nextInt(8),
			_pos.y + _rand.nextInt(4) - _rand.nextInt(4),
			_pos.z + _rand.nextInt(8) - _rand.nextInt(8),
		};
		// If air with netherrack underneath, generate
		if (_world.getBlockId(test_pos) == BLOCK_AIR && _world.getBlockId(test_pos + Int3{ 0, -1, 0 }) == BLOCK_NETHERRACK)
			_world.setBlock(test_pos, BLOCK_FIRE);
	}
	return true;
}

//  GenerateNetherGlowstone
bool FeatureGenerator::GenerateNetherGlowstone(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	// Exit if tested block isn't air
	if (_world.getBlockId(_pos) != BLOCK_AIR)
		return false;
	// Exit if block above tested block isn't netherrack
	if (_world.getBlockId(_pos + Int3{ 0, 1, 0 }) != BLOCK_NETHERRACK)
		return false;
	_world.setBlock(_pos, BLOCK_GLOWSTONE);
	for (int i = 0; i < 1500; ++i) {
		Int3 test_pos{
			_pos.x + _rand.nextInt(8) - _rand.nextInt(8),
			_pos.y - _rand.nextInt(12),
			_pos.z + _rand.nextInt(8) - _rand.nextInt(8),
		};
		// Skip non-air blocks
		if (_world.getBlockId(test_pos) != BLOCK_AIR)
			continue;
		int adjacent_glowstone_count = 0;
		// Check for adjacent glowstone blocks
		for (int direction = 0; direction < 6; ++direction) {
			BlockType adjacent_block = BLOCK_AIR;
			switch (direction) {
			case 0:
				adjacent_block = _world.getBlockId(test_pos + Int3{ -1, 0, 0 });
				break;
			case 1:
				adjacent_block = _world.getBlockId(test_pos + Int3{ +1, 0, 0 });
				break;
			case 2:
				adjacent_block = _world.getBlockId(test_pos + Int3{ 0, -1, 0 });
				break;
			case 3:
				adjacent_block = _world.getBlockId(test_pos + Int3{ 0, +1, 0 });
				break;
			case 4:
				adjacent_block = _world.getBlockId(test_pos + Int3{ 0, 0, -1 });
				break;
			case 5:
				adjacent_block = _world.getBlockId(test_pos + Int3{ 0, 0, +1 });
				break;
			default:
				break;
			}
			if (adjacent_block == BLOCK_GLOWSTONE)
				adjacent_glowstone_count++;
		}
		// If onle one adjacent glowstone exists, place another
		if (adjacent_glowstone_count == 1)
			_world.setBlock(test_pos, BLOCK_GLOWSTONE);
	}
	return true;
}