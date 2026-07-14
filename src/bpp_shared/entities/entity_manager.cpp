/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "entity_manager.h"
#include "world.h"

void EntityManager::removeEntity(EntityId id) {
	// Find the entity for this ID
	auto it = std::find_if(entities.begin(), entities.end(),
	                       [id](const std::shared_ptr<Entity>& e) { return e->id == id; });
	if (it == entities.end())
		return; // Not found, nothing to do

	std::shared_ptr<Entity> entity = *it;

	// Remove from its bucket
	auto& container = entityContainers[{ entity->bucketPos.x, entity->bucketPos.y }];
	auto& bucket = container.buckets[entity->bucketPos.z];
	bucket.entities.erase(std::remove_if(bucket.entities.begin(), bucket.entities.end(),
	                                     [&entity](const std::weak_ptr<Entity>& weak) {
		                                     auto locked = weak.lock();
		                                     return !locked || locked == entity;
	                                     }),
	                      bucket.entities.end());

	// Remove from the master list
	entities.erase(it);
	
	// Unbind ourselves
	entity->entityManager = nullptr;

	// Set as dead for cleanup
	entity->isDead = true;
	if (onEntityDespawn)
		onEntityDespawn(entity);
}

void EntityManager::addEntity(std::shared_ptr<Entity> entity, EntityId forceEntityId) {
	if (!world) {
		GlobalLogger().error << "Attempted to add an entity before EntityManager was bound to a world!\n";
		return;
	}
	entity->id = forceEntityId == -1 ? getNextEntityId()
	                                 : forceEntityId; // Assign an ID if we weren't forced to use one
	entity->world = world; // Bind the world pointer so the entity can interact with the world
	entity->entityManager = this;
	entity->dim = world->thisDimension;

	// Register the entity into its initial bucket
	entity->bucketPos = computeBucketPos(entity->posX, entity->posY, entity->posZ);
	auto& container = entityContainers[{ entity->bucketPos.x, entity->bucketPos.y }];
	container.buckets[entity->bucketPos.z].entities.push_back(entity);

	entities.push_back(std::move(entity));
	if (onEntitySpawn)
		onEntitySpawn(entities.back());
}

void EntityManager::tick() {
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

std::vector<std::shared_ptr<Entity>> EntityManager::getEntitiesWithinAABBExcluding(AABB& box, EntityId entityId) {
	// Get all entities within an AABB excluding this entity id
	auto entities = getEntitiesWithinAABB(box);
	entities.erase(std::remove_if(entities.begin(), entities.end(),
	                              [entityId](std::shared_ptr<Entity> entity) { return entity->id == entityId; }),
	               entities.end());
	return entities;
}

std::vector<std::shared_ptr<Entity>> EntityManager::getEntitiesWithinAABB(AABB& box) {
	// Get all entities within an AABB
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