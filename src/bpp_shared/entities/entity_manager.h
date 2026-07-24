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
	EntityId nextEntityId = 2; // Minecraft seems to reserve 0 and 1
	std::vector<std::shared_ptr<Entity>> entities;
	std::unordered_map<Int2, EntityContainer> entityContainers;
	WorldManager* world = nullptr; // we need to bind a pointer to this later

	// Callbacks that we can link into
	std::function<void(std::shared_ptr<Entity>)> onEntitySpawn;
	std::function<void(std::shared_ptr<Entity>)> onEntityDespawn;

	static Int3 ComputeBucketPos(Vec3 _position) {
		Int3 bucketPos = { int(MathHelper::FloorDouble(_position.x / 16.0)),
			               int(MathHelper::FloorDouble(_position.z / 16.0)),
			               int(MathHelper::FloorDouble(_position.y / 16.0)) };

		// Entity collisions below and above the world are just gonna be inefficient
		bucketPos.z = std::max(0, bucketPos.z);
		bucketPos.z = std::min(9, bucketPos.z);
		return bucketPos;
	}

	std::vector<std::shared_ptr<Entity>> GetEntitiesWithinAabbExcluding(const AABB& _box, const EntityId _entityId);
	std::vector<std::shared_ptr<Entity>> GetEntitiesWithinAabb(const AABB& _box);
	std::vector<std::shared_ptr<Entity>> GetEntitiesWithinAabbExcludingTypes(
	    const AABB& _box, const std::vector<EntityType>& _excludedTypes);
	void Tick();
	void AddEntity(std::shared_ptr<Entity> _entity, EntityId _forceEntityId = -1);
	void RemoveEntity(EntityId _id);
	bool ChunkHasEntities(Int2 _cpos) {
		auto& container = this->entityContainers[_cpos];
		for (size_t i = 0; i < container.buckets.size(); i++) {
			auto& bucket = container.buckets[i];
			if (bucket.entities.size() > 0)
				return true;
		}
		return false;
	}
	std::vector<Tag> CollectEntitiesForSave(Int2 _cpos, bool _clearCollectedEntities = false);
	std::shared_ptr<Entity> GetEntityByIdShared(EntityId _id) {
		for (auto& entity : entities) {
			if (entity->id == _id)
				return entity;
		}
		return nullptr;
	}
	void CreateEntityFromNbt(Tag& _nbt);

	EntityId GetNextEntityId() {
		return nextEntityId++;
	}

	std::optional<std::string> GetEntityNbtId(EntityType _type);
};