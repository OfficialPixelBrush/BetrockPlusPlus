/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "spawner.h"
#include "entities/entity_animal.h"
#include "entities/entity_chicken.h"
#include "entities/entity_cow.h"
#include "entities/entity_pig.h"
#include "entities/entity_sheep.h"
#include "world.h"

struct AnimalSpawnEntry {
	std::function<std::shared_ptr<AnimalEntity>()> factory;
	int weight;
};

// TODO: This should be base on biome!
static const std::vector<AnimalSpawnEntry> PASSIVE_SPAWN_LIST = {
	{ []() { return std::make_shared<PigEntity>(); }, 10 },
	{ []() { return std::make_shared<CowEntity>(); }, 8 },
	{ []() { return std::make_shared<ChickenEntity>(); }, 10 },
	{ []() { return std::make_shared<SheepEntity>(); }, 12 },
};

// Picks one entry from the weighted list
static const AnimalSpawnEntry& PickWeighted(Java::Random& _rand) {
	int total = 0;
	for (auto& entry : PASSIVE_SPAWN_LIST)
		total += entry.weight;

	int roll = _rand.NextInt(total);
	for (auto& entry : PASSIVE_SPAWN_LIST) {
		roll -= entry.weight;
		if (roll < 0)
			return entry;
	}
	return PASSIVE_SPAWN_LIST.back(); // unreachable in practice
}

int EntitySpawner::GetPassiveCount(WorldManager& _world) {
	int count = _world.entityManager.CountEntitiesOfType(EntityType::CHICKEN) +
	            _world.entityManager.CountEntitiesOfType(EntityType::COW) +
	            _world.entityManager.CountEntitiesOfType(EntityType::PIG) +
	            _world.entityManager.CountEntitiesOfType(EntityType::SHEEP);
	return count;
}

void EntitySpawner::TrySpawnEntities(WorldManager& _world, const std::vector<ClientPosition>& _players) {
	BuildActiveSet(_world, _players);

	int mobCap = 15 * chunksToSpawnIn.size() / 256;

	if (GetPassiveCount(_world) >= mobCap || chunksToSpawnIn.empty()) {
		return;
	}

	for (auto& cpos : chunksToSpawnIn) {
		Int3 anchorPos = { cpos.x * 16 + _world.rand.NextInt(16), _world.rand.NextInt(128),
			               cpos.z * 16 + _world.rand.NextInt(16) };

		if (_world.IsBlockNormalCube(anchorPos) || _world.GetMaterial(anchorPos).isSolid)
			continue; // Discard this chunk

		const AnimalSpawnEntry& picked = PickWeighted(_world.rand);

		// Spawn 3 groups x 4 pos
		int spawnedThisCluster = 0;
		for (int group = 0; group < 3 && spawnedThisCluster < 4; group++) {
			Int3 pos = anchorPos;

			for (int attempt = 0; attempt < 4; attempt++) {
				pos.x += _world.rand.NextInt(6) - _world.rand.NextInt(6);
				pos.z += _world.rand.NextInt(6) - _world.rand.NextInt(6);

				// Distance checks
				if (_world.entityManager.GetClosestPlayerWithin(pos, 24.0) != nullptr)
					continue;
				if (pos.Distance(_world.GetSpawnPoint(false)) < 24.0)
					continue;

				auto candidate = picked.factory();
				candidate->world = &_world;
				candidate->entityManager = &_world.entityManager;

				Vec3 position = { pos.x + 0.5, double(pos.y), pos.z + 0.5 };
				float rotationYaw = _world.rand.NextFloat() * 360.0f;

				candidate->Teleport(position, { rotationYaw, 0.0 });
				if (!candidate->CanSpawnAt(pos))
					continue;

				_world.entityManager.AddEntity(candidate);
				spawnedThisCluster++;

				if (spawnedThisCluster >= 4)
					break;
			}
		}
	}
}

void EntitySpawner::BuildActiveSet(WorldManager& _world, const std::vector<ClientPosition>& _players) {
	int range = std::min(8, _world.GetSimulationDistance());
	chunksToSpawnIn.clear();

	for (auto& player : _players) {
		Int2 playerChunkPos = player.GetChunkPos();
		for (int dx = -range; dx <= range; dx++) {
			for (int dz = -range; dz <= range; dz++) {
				Int2 chunkPos = { playerChunkPos.x + dx, playerChunkPos.z + dz };
				auto chunk = _world.GetChunkRaw(chunkPos);
				if (chunk)
					chunksToSpawnIn.insert(chunkPos);
			}
		}
	}
}