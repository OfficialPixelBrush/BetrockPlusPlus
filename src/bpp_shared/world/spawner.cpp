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
#include "entities/entity_creeper.h"
#include "entities/entity_mob.h"
#include "entities/entity_pig.h"
#include "entities/entity_sheep.h"
#include "entities/entity_skeleton.h"
#include "entities/entity_spider.h"
#include "entities/entity_zombie.h"
#include "world.h"

EntitySpawner::EntitySpawner() {
	// Passive (need to make this biome dependent)
	categories.push_back({
	    .spawnList = {
	        { []() { return std::make_shared<PigEntity>(); }, 10 },
	        { []() { return std::make_shared<CowEntity>(); }, 8 },
	        { []() { return std::make_shared<ChickenEntity>(); }, 10 },
	        { []() { return std::make_shared<SheepEntity>(); }, 12 },
	    },
	    .cap = 15,
	});

	// Hostile
	categories.push_back({
	    .spawnList = {
	        { []() { return std::make_shared<ZombieEntity>(); }, 10 },
			{ []() { return std::make_shared<CreeperEntity>(); }, 10 },
			{ []() { return std::make_shared<SpiderEntity>(); }, 10 },
			{ []() { return std::make_shared<SkeletonEntity>(); }, 10 },
	    },
	    .cap = 70,
	});
}

// Picks one entry from the weighted list
static const SpawnEntry& PickWeighted(Java::Random& _rand, SpawnCategory& _category) {
	int total = 0;
	for (auto& entry : _category.spawnList)
		total += entry.weight;

	int roll = _rand.NextInt(total);
	for (auto& entry : _category.spawnList) {
		roll -= entry.weight;
		if (roll < 0)
			return entry;
	}
	return _category.spawnList.back(); // unreachable in practice
}

static bool IsValidSpawnBlock(WorldManager& _world, Int3 _pos) {
	return _world.IsBlockNormalCube({ _pos.x, _pos.y - 1, _pos.z }) && !_world.IsBlockNormalCube(_pos) &&
	       !_world.GetMaterial(_pos).isLiquid && !_world.IsBlockNormalCube({ _pos.x, _pos.y + 1, _pos.z });
}

int EntitySpawner::GetCategoryCount(WorldManager& _world, SpawnCategory& _category) {
	int count = 0;
	for (auto& spawnType : _category.spawnList) {
		count += _world.entityManager.CountEntitiesOfType(spawnType.factory()->type);
	}
	return count;
}

void EntitySpawner::TrySpawnEntities(WorldManager& _world, const std::vector<ClientPosition>& _players) {
	BuildActiveSet(_world, _players);

	if (chunksToSpawnIn.empty())
		return;

	for (auto& category : this->categories) {
		int mobCap = category.cap * int(chunksToSpawnIn.size()) / 256;

		if (GetCategoryCount(_world, category) >= mobCap)
			continue;

		for (auto& cpos : chunksToSpawnIn) {
			Int3 anchorPos = { cpos.x * 16 + _world.rand.NextInt(16), _world.rand.NextInt(128),
				               cpos.z * 16 + _world.rand.NextInt(16) };

			if (_world.IsBlockNormalCube(anchorPos) || _world.GetMaterial(anchorPos).isSolid)
				continue; // Discard this chunk

			const SpawnEntry& picked = PickWeighted(_world.rand, category);

			int spawnedThisCluster = 0;
			for (int group = 0; group < 3 && spawnedThisCluster < 4; group++) {
				Int3 pos = anchorPos;

				for (int attempt = 0; attempt < 4; attempt++) {
					pos.x += _world.rand.NextInt(6) - _world.rand.NextInt(6);
					pos.z += _world.rand.NextInt(6) - _world.rand.NextInt(6);

					if (!IsValidSpawnBlock(_world, pos))
						continue;
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
}

void EntitySpawner::BuildActiveSet(WorldManager& _world, const std::vector<ClientPosition>& _players) {
	int range = std::min(8, _world.GetSimulationDistance());
	chunksToSpawnIn.clear();

	for (auto& player : _players) {
		Int2 playerChunkPos = player.GetChunkPos();
		for (int dx = -range; dx <= range; dx++) {
			for (int dz = -range; dz <= range; dz++) {
				Int2 chunkPos = { playerChunkPos.x + dx, playerChunkPos.z + dz };
				auto chunk = _world.GetChunk(chunkPos);
				if (chunk && chunk->state.load() == ChunkState::Populated)
					chunksToSpawnIn.insert(chunkPos);
			}
		}
	}
}