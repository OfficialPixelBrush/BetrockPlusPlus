/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "numeric_structs.h"
#include <unordered_set>
#include <vector>

// For spawning entities in the world like mobs and animals
struct WorldManager;
struct Chunk;
struct ClientPosition;
struct EntitySpawner {
	std::unordered_set<Int32_2> chunksToSpawnIn;

	void TrySpawnEntities(WorldManager& _world, const std::vector<ClientPosition>& _players);
	void BuildActiveSet(WorldManager& _world, const std::vector<ClientPosition>& _players);
	int GetPassiveCount(WorldManager& _world);
};