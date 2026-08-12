/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "numeric_structs.h"
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

class WorldManager;
struct Chunk;
struct ClientPosition;
struct MobEntity;

struct SpawnEntry {
	std::function<std::shared_ptr<MobEntity>()> factory;
	int weight;
};

struct SpawnCategory {
	std::vector<SpawnEntry> spawnList = {};
	int cap = 15;
};

// For spawning entities in the world like mobs and animals
struct EntitySpawner {
	std::unordered_set<Int32_2> chunksToSpawnIn;
	std::vector<SpawnCategory> categories;

	EntitySpawner();

	void TrySpawnEntities(WorldManager& _world, const std::vector<ClientPosition>& _players);
	void BuildActiveSet(WorldManager& _world, const std::vector<ClientPosition>& _players);
	int GetCategoryCount(WorldManager& _world, SpawnCategory& _category);
};