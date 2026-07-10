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

	static Int3 computeBucketPos(double posX, double posY, double posZ) {
		Int3 pos = { int(MathHelper::floor_double(posX / 16.0)), int(MathHelper::floor_double(posZ / 16.0)),
			         int(MathHelper::floor_double(posY / 16.0)) };

		// Entity collisions below and above the world are just gonna be inefficient
		pos.z = std::max(0, pos.z);
		pos.z = std::min(9, pos.z);
		return pos;
	}

	std::vector<std::shared_ptr<Entity>> getEntitiesWithinAABBExcluding(AABB& box, EntityId entityId) {
		auto entities = getEntitiesWithinAABB(box);
		entities.erase(std::remove_if(entities.begin(), entities.end(),
		                              [entityId](std::shared_ptr<Entity> entity) { return entity->id == entityId; }),
		               entities.end());
		return entities;
	}

	std::vector<std::shared_ptr<Entity>> getEntitiesWithinAABB(AABB& box) {
		std::vector<std::shared_ptr<Entity>> collidingEntities;

		// Normalize to block coordinates
		int blockMinX = MathHelper::floor_double((box.minX - 2.0) / 16.0);
		int blockMinZ = MathHelper::floor_double((box.minZ - 2.0) / 16.0);
		int blockMaxX = MathHelper::floor_double((box.maxX + 2.0) / 16.0);
		int blockMaxZ = MathHelper::floor_double((box.maxZ + 2.0) / 16.0);

		// Get our start and end bucket
		int bucketMinY = MathHelper::floor_double((box.minY - 2.0) / 16.0);
		int bucketMaxY = MathHelper::floor_double((box.maxY + 2.0) / 16.0);
		bucketMinY = std::max(0, bucketMinY);
		bucketMinY = std::min(9, bucketMinY);
		bucketMaxY = std::max(0, bucketMaxY);
		bucketMaxY = std::min(9, bucketMaxY);

		// Go through each block position
		for (int x = blockMinX; x <= blockMaxX; x++) {
			for (int z = blockMinZ; z <= blockMaxZ; z++) {
				auto& container = entityContainers[{ x, z }];
				for (int by = bucketMinY; by <= bucketMaxY; by++) {
					// Get every entity within every bucket
					for (int i = 0; i < container.buckets[by].entities.size(); i++) {
						// Make sure the weak ptr is still valid
						auto& entityPtrWeak = container.buckets[by].entities[i];
						if (auto entityPtrShared = entityPtrWeak.lock())
							collidingEntities.push_back(entityPtrShared);
					}
				}
			}
		}
		return collidingEntities;
	}

	void tick() {
		// Make a copy so we aren't modifying the vector while iterating over it
		std::vector<std::shared_ptr<Entity>> copy = entities;

		// Tick EVERY entity
		for (std::shared_ptr<Entity> entity : copy) {
			// Remove dead entities from the system
			if (entity->isDead) {
				entity->world = nullptr;
				entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
				auto& container = entityContainers[{ entity->bucketPos.x, entity->bucketPos.y }];
				auto& b = container.buckets[entity->bucketPos.z];
				b.entities.erase(std::remove_if(b.entities.begin(), b.entities.end(),
				                                [&entity](const std::weak_ptr<Entity>& weak) {
					                                auto locked = weak.lock();
					                                return !locked ||
					                                       locked == entity; // Remove if expired or matches our entity
				                                }),
				                 b.entities.end());

				if (onEntityDespawn)
					onEntityDespawn(entity);
				continue;
			}
			entity->tick();

			// Check to see if this entity went into another container or bucket
			Int3 newBucketPos = computeBucketPos(entity->posX, entity->posY, entity->posZ);

			if (newBucketPos != entity->bucketPos) {
				// Remove from the old bucket
				auto& oldContainer = entityContainers[{ entity->bucketPos.x, entity->bucketPos.y }];
				auto& b = oldContainer.buckets[entity->bucketPos.z];
				b.entities.erase(std::remove_if(b.entities.begin(), b.entities.end(),
				                                [&entity](const std::weak_ptr<Entity>& weak) {
					                                auto locked = weak.lock();
					                                return !locked ||
					                                       locked == entity; // Remove if expired or matches our entity
				                                }),
				                 b.entities.end());

				// Put in the new bucket
				auto& newContainer = entityContainers[{ newBucketPos.x, newBucketPos.y }];
				auto& newB = newContainer.buckets[newBucketPos.z];
				newB.entities.push_back(entity);
				entity->bucketPos = newBucketPos;
			}
		}
		copy.clear(); // Clear the copy to free memory
	}

	void addEntity(std::shared_ptr<Entity> entity, EntityId forceEntityId = -1) {
		if (!world) {
			GlobalLogger().error << "Attempted to add an entity before EntityManager was bound to a world!\n";
			return;
		}
		entity->id = forceEntityId == -1 ? getNextEntityId()
		                                 : forceEntityId; // Assign an ID if we weren't forced to use one
		entity->world = world; // Bind the world pointer so the entity can interact with the world
		entity->entityManager = this;

		// Register the entity into its initial bucket
		entity->bucketPos = computeBucketPos(entity->posX, entity->posY, entity->posZ);
		auto& container = entityContainers[{ entity->bucketPos.x, entity->bucketPos.y }];
		container.buckets[entity->bucketPos.z].entities.push_back(entity);

		entities.push_back(std::move(entity));
		if (onEntitySpawn)
			onEntitySpawn(entities.back());
	}

	EntityId getNextEntityId() {
		return nextEntityId++;
	}
};