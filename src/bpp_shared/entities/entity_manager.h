/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "entity.h"
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
	int64_t nextEntityId = 2; // Minecraft seems to reserve 0 and 1
	std::vector<std::shared_ptr<Entity>> entities;
	std::unordered_map<Int2, EntityContainer> entityContainers;
	WorldManager* world = nullptr; // we need to bind a pointer to this later

	void tick() {
		// Tick EVERY entity
		for (std::shared_ptr<Entity> entity : entities) {
			entity->tick();

			// Check to see if this entity went into another container or bucket
			Int3 newBucketPos = { int(entity->posX / 16.0), int(entity->posZ / 16.0), int(entity->posY / 16.0) };

			// Entity collisions below and above the world are just gonna be inefficient
			if (newBucketPos.z < 0)
				newBucketPos.z = 0;
			else if (newBucketPos.z > 9)
				newBucketPos.z = 9;

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
	}

	void addEntity(std::shared_ptr<Entity> entity, int64_t forceEntityId = -1) {
		if (!world) {
			GlobalLogger().error << "Attempted to add an entity before EntityManager was bound to a world!\n";
			return;
		}
		entity->id = forceEntityId == -1 ? getNextEntityId()
		                                 : forceEntityId; // Assign an ID if we weren't forced to use one
		entity->world = world; // Bind the world pointer so the entity can interact with the world
		entities.push_back(std::move(entity));
	}

	int64_t getNextEntityId() {
		return nextEntityId++;
	}
};
