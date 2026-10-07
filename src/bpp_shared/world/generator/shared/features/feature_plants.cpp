/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "../feature_gen.h"

//  Attempts to generate flower/mushroom patches
bool FeatureGenerator::GenerateFlowers(BlockType _type, WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	bool isMushroom = (_type == BLOCK_MUSHROOM_BROWN || _type == BLOCK_MUSHROOM_RED);

	for (int32_t i = 0; i < 64; ++i) {
		int32_t x = _pos.x + _rand.NextInt(8) - _rand.NextInt(8);
		int32_t y = _pos.y + _rand.NextInt(4) - _rand.NextInt(4);
		int32_t z = _pos.z + _rand.NextInt(8) - _rand.NextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT)
			continue;
		if (_world.GetBlockId({ x, y, z }) != BLOCK_AIR)
			continue;

		if (isMushroom) {
			if (Blocks::CanMushroomSurviveAt(_world, { x, y, z }))
				_world.SetBlock({ x, y, z }, _type);
		} else {
			if (Blocks::CanGenericPlantSurviveAt(_world, { x, y, z }))
				_world.SetBlock({ x, y, z }, _type);
		}
	}
	return true;
}

//  Attempts to generate tallgrass patches
bool FeatureGenerator::GenerateTallgrass(uint8_t _meta, WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	while (_pos.y > 0) {
		BlockType b = _world.GetBlockId({ _pos.x, _pos.y, _pos.z });
		if (b != BLOCK_AIR && b != BLOCK_LEAVES)
			break;
		--_pos.y;
	}

	for (int32_t i = 0; i < 128; ++i) {
		int32_t x = _pos.x + _rand.NextInt(8) - _rand.NextInt(8);
		int32_t y = _pos.y + _rand.NextInt(4) - _rand.NextInt(4);
		int32_t z = _pos.z + _rand.NextInt(8) - _rand.NextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT)
			continue;
		if (_world.GetBlockId({ x, y, z }) != BLOCK_AIR)
			continue;
		if (Blocks::CanGenericPlantSurviveAt(_world, { x, y, z }))
			_world.SetBlock({ x, y, z }, BLOCK_TALLGRASS, _meta);
	}
	return true;
}

//  Attempts to generate deadbush patches
bool FeatureGenerator::GenerateDeadbush(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	while (_pos.y > 0) {
		BlockType b = _world.GetBlockId({ _pos.x, _pos.y, _pos.z });
		if (b != BLOCK_AIR && b != BLOCK_LEAVES)
			break;
		--_pos.y;
	}

	for (int32_t i = 0; i < 4; ++i) {
		int32_t x = _pos.x + _rand.NextInt(8) - _rand.NextInt(8);
		int32_t y = _pos.y + _rand.NextInt(4) - _rand.NextInt(4);
		int32_t z = _pos.z + _rand.NextInt(8) - _rand.NextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT)
			continue;
		if (_world.GetBlockId({ x, y, z }) == BLOCK_AIR && _world.GetBlockId({ x, y - 1, z }) == BLOCK_SAND)
			_world.SetBlock({ x, y, z }, BLOCK_DEADBUSH);
	}
	return true;
}

//  Attempts to generate sugarcane patches
bool FeatureGenerator::GenerateSugarcane(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	for (int32_t i = 0; i < 20; ++i) {
		int32_t x = _pos.x + _rand.NextInt(4) - _rand.NextInt(4);
		int32_t y = _pos.y; // Y is fixed across all attempts
		int32_t z = _pos.z + _rand.NextInt(4) - _rand.NextInt(4);
		if (_world.GetBlockId({ x, y, z }) != BLOCK_AIR)
			continue;
		if (!Blocks::CanSugarcaneSurviveAt(_world, { x, y, z }))
			continue;

		int32_t height = 2 + _rand.NextInt(_rand.NextInt(3) + 1);
		for (int32_t h = 0; h < height; ++h) {
			if (_world.GetBlockId({ x, y + h, z }) != BLOCK_AIR)
				break;
			if (!Blocks::CanSugarcaneSurviveAt(_world, { x, y + h, z }))
				break;
			_world.SetBlock({ x, y + h, z }, BLOCK_SUGARCANE);
		}
	}
	return true;
}

//  Attempts to generate pumpkin patches
bool FeatureGenerator::GeneratePumpkins(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	for (int32_t i = 0; i < 64; ++i) {
		int32_t x = _pos.x + _rand.NextInt(8) - _rand.NextInt(8);
		int32_t y = _pos.y + _rand.NextInt(4) - _rand.NextInt(4);
		int32_t z = _pos.z + _rand.NextInt(8) - _rand.NextInt(8);
		if (y < 0 || y >= CHUNK_HEIGHT)
			continue;
		if (_world.GetBlockId({ x, y, z }) != BLOCK_AIR)
			continue;
		if (_world.GetBlockId({ x, y - 1, z }) != BLOCK_GRASS)
			continue;
		// canPlaceBlockAt: no adjacent pumpkins on cardinal sides
		if (_world.GetBlockId({ x - 1, y, z }) == BLOCK_PUMPKIN)
			continue;
		if (_world.GetBlockId({ x + 1, y, z }) == BLOCK_PUMPKIN)
			continue;
		if (_world.GetBlockId({ x, y, z - 1 }) == BLOCK_PUMPKIN)
			continue;
		if (_world.GetBlockId({ x, y, z + 1 }) == BLOCK_PUMPKIN)
			continue;
		_world.SetBlock({ x, y, z }, BLOCK_PUMPKIN, uint8_t(_rand.NextInt(4)));
	}
	return true;
}

//  Attempts to generate cacti patches
bool FeatureGenerator::GenerateCacti(WorldWrapper& _world, Java::Random& _rand, Int3 _pos) {
	for (int32_t i = 0; i < 10; ++i) {
		int32_t x = _pos.x + _rand.NextInt(8) - _rand.NextInt(8);
		int32_t y = _pos.y + _rand.NextInt(4) - _rand.NextInt(4);
		int32_t z = _pos.z + _rand.NextInt(8) - _rand.NextInt(8);
		if (_world.GetBlockId({ x, y, z }) != BLOCK_AIR)
			continue;

		int32_t height = 1 + _rand.NextInt(_rand.NextInt(3) + 1);
		for (int32_t h = 0; h < height; ++h) {
			if (Blocks::CanCactusSurviveAt(_world, { x, y + h, z }))
				_world.SetBlock({ x, y + h, z }, BLOCK_CACTUS);
		}
	}
	return true;
}