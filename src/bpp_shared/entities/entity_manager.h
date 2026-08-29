/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "entity.h"
#include "helpers/java/java_math.h"
#include "logger/logger.h"
#include <functional>
#include <memory>

struct EntityBucket {
	// 16 blocks tall
	std::vector<std::weak_ptr<Entity>> entities;
};

struct EntityContainer {
	Int2 bucketPos = { 0, 0 };
	std::array<EntityBucket, 10>
	    buckets; // 0 = lowest bucket (below the world), 1 = Y lvl 0; 8 = y lvl 127, 9 = above the world
};

// For ticking all entities and keeping track of them in the world
struct EntityManager {
	EntityId* nextEntityId = nullptr; // Minecraft seems to reserve 0 and 1
	std::vector<std::shared_ptr<Entity>> entities;
	std::vector<std::weak_ptr<Entity>> players;
	std::unordered_map<Int2, EntityContainer> entityContainers;
	WorldManager* world = nullptr; // we need to bind a pointer to this later

	// Callbacks that we can link into
	std::function<void(std::shared_ptr<Entity>)> onEntitySpawn;
	std::function<void(std::shared_ptr<Entity>)> onEntityDespawn;

	std::vector<std::shared_ptr<Entity>> GetEntitiesWithinAabbExcluding(const AABB& _box, const EntityId _entityId);
	std::vector<std::shared_ptr<Entity>> GetEntitiesWithinAabb(const AABB& _box);
	std::vector<std::shared_ptr<Entity>> GetEntitiesWithinAabbExcludingTypes(
	    const AABB& _box, const std::vector<EntityType>& _excludedTypes);
	std::vector<Tag> CollectEntitiesForSave(Int2 _cpos, bool _clearCollectedEntities = false);
	std::optional<std::string> GetEntityNbtId(EntityType _type);
	void Tick();
	void AddEntity(std::shared_ptr<Entity> _entity, EntityId _forceEntityId = -1);
	void RemoveEntity(EntityId _id);
	void CreateEntityFromNbt(Tag& _nbt);

private:
	void TickEntityAndPassenger(const std::shared_ptr<Entity>& _entity);

public:
	static Int3 ComputeBucketPos(Vec3 _position) {
		Int3 bucketPos = { MathHelper::FloorDouble(_position.x / 16.0), MathHelper::FloorDouble(_position.z / 16.0),
			               MathHelper::FloorDouble(_position.y / 16.0) };

		// Entity collisions below and above the world are just gonna be inefficient
		bucketPos.z = std::max(0, bucketPos.z);
		bucketPos.z = std::min(9, bucketPos.z);
		return bucketPos;
	}
	static bool IsContainerEmpty(const EntityContainer& _container) {
		for (const auto& bucket : _container.buckets) {
			if (!bucket.entities.empty())
				return false;
		}
		return true;
	}

	bool ChunkHasEntities(Int2 _cpos) const {
		auto it = entityContainers.find(_cpos);
		if (it == entityContainers.end())
			return false;
		return !IsContainerEmpty(it->second);
	}

	// Drop empty containers so lookups never permanently inflate the map.
	void PruneEmptyContainer(Int2 _cpos) {
		auto it = entityContainers.find(_cpos);
		if (it != entityContainers.end() && IsContainerEmpty(it->second))
			entityContainers.erase(it);
	}

	void EraseContainer(Int2 _cpos) {
		entityContainers.erase(_cpos);
	}
	std::shared_ptr<Entity> GetEntityByIdShared(EntityId _id) {
		for (auto& entity : entities) {
			if (entity->id == _id)
				return entity;
		}
		return nullptr;
	}
	EntityId GetNextEntityId() {
		if (!nextEntityId)
			return -1;
		return (*nextEntityId)++;
	}
	int CountEntitiesOfType(EntityType _type) {
		int count = 0;
		for (auto& entity : entities) {
			if (entity->type == _type)
				count++;
		}
		return count;
	}
	template <typename T>
	std::shared_ptr<Entity> GetClosestPlayerWithin(T _pos, int _distance) {
		double minDist = 0;
		std::shared_ptr<Entity> closestPlayer = nullptr;

		for (const auto& playerWeak : players) {
			if (auto player = playerWeak.lock()) {
				double dist = Vec3{ double(_pos.x), double(_pos.y), double(_pos.z) }.Distance(player->position);
				if ((_distance < 0 || dist <= _distance) && (minDist == 0 || dist < minDist)) {
					minDist = dist;
					closestPlayer = player;
					if (dist == 0)
						break;
				}
			}
		}

		return closestPlayer;
	}
};