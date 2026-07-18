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
	std::vector<std::weak_ptr<Entity>> m_entities;
};

struct EntityContainer {
	Int2 bucketPos = { 0, 0 };
	std::array<EntityBucket, 10>
	    buckets; // 0 = lowest bucket (below the world), 1 = Y lvl 0; 8 = y lvl 127, 9 = above the world
};

// For ticking all entities and keeping track of them in the world
struct EntityManager {
	EntityId m_nextEntityId = 2; // Minecraft seems to reserve 0 and 1
	std::vector<std::shared_ptr<Entity>> m_entities;
	std::unordered_map<Int2, EntityContainer> m_entityContainers;
	WorldManager* m_world = nullptr; // we need to bind a pointer to this later

	// Callbacks that we can link into
	std::function<void(std::shared_ptr<Entity>)> onEntitySpawn;
	std::function<void(std::shared_ptr<Entity>)> onEntityDespawn;

	static Int3 computeBucketPos(Vec3 position) {
		Int3 bucketPos = { int(MathHelper::floor_double(position.x / 16.0)),
			               int(MathHelper::floor_double(position.z / 16.0)),
			               int(MathHelper::floor_double(position.y / 16.0)) };

		// Entity collisions below and above the world are just gonna be inefficient
		bucketPos.z = std::max(0, bucketPos.z);
		bucketPos.z = std::min(9, bucketPos.z);
		return bucketPos;
	}

	std::vector<std::shared_ptr<Entity>> getEntitiesWithinAABBExcluding(const AABB& box, EntityId entityId);
	std::vector<std::shared_ptr<Entity>> getEntitiesWithinAABB(const AABB& box);
	void tick();
	void addEntity(std::shared_ptr<Entity> entity, EntityId forceEntityId = -1);
	void removeEntity(EntityId id);
	bool chunkHasEntities(Int2 cpos) {
		auto& container = this->m_entityContainers[cpos];
		for (size_t i = 0; i < container.buckets.size(); i++) {
			auto& bucket = container.buckets[i];
			if (bucket.m_entities.size() > 0)
				return true;
		}
		return false;
	}
	std::vector<Tag> collectEntitiesForSave(Int2 cpos, bool clearCollectedEntities = false);
	std::shared_ptr<Entity> getEntityByIdShared(EntityId id) {
		for (auto& entity : m_entities) {
			if (entity->id == id)
				return entity;
		}
		return nullptr;
	}
	void createEntityFromNBT(Tag& nbt);

	EntityId getNextEntityId() {
		return m_nextEntityId++;
	}

	std::optional<std::string> getEntityNbtId(EntityType type);
};