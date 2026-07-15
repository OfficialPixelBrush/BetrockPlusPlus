/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "entity_manager.h"
#include "entity_item.h"
#include "world.h"

void EntityManager::removeEntity(EntityId id) {
	// Find the entity for this ID
	auto it = std::find_if(m_entities.begin(), m_entities.end(),
	                       [id](const std::shared_ptr<Entity>& e) { return e->id == id; });
	if (it == m_entities.end())
		return; // Not found, nothing to do

	std::shared_ptr<Entity> entity = *it;

	// Remove from its bucket
	auto& container = m_entityContainers[{ entity->bucketPos.x, entity->bucketPos.y }];
	auto& bucket = container.buckets[entity->bucketPos.z];
	bucket.m_entities.erase(std::remove_if(bucket.m_entities.begin(), bucket.m_entities.end(),
	                                       [&entity](const std::weak_ptr<Entity>& weak) {
		                                       auto locked = weak.lock();
		                                       return !locked || locked == entity;
	                                       }),
	                        bucket.m_entities.end());

	// Remove from the master list
	m_entities.erase(it);

	// Unbind ourselves
	entity->entityManager = nullptr;

	// Set as dead for cleanup
	entity->isDead = true;
	if (onEntityDespawn)
		onEntityDespawn(entity);
}

void EntityManager::addEntity(std::shared_ptr<Entity> entity, EntityId forceEntityId) {
	if (!m_world) {
		GlobalLogger().error << "Attempted to add an entity before EntityManager was bound to a m_world!\n";
		return;
	}
	entity->id = forceEntityId == -1 ? getNextEntityId()
	                                 : forceEntityId; // Assign an ID if we weren't forced to use one
	entity->world = m_world; // Bind the m_world pointer so the entity can interact with the m_world
	entity->entityManager = this;
	entity->dim = m_world->thisDimension;

	// Register the entity into its initial bucket
	entity->bucketPos = computeBucketPos(entity->posX, entity->posY, entity->posZ);
	auto& container = m_entityContainers[{ entity->bucketPos.x, entity->bucketPos.y }];
	container.buckets[entity->bucketPos.z].m_entities.push_back(entity);

	m_entities.push_back(std::move(entity));
	if (onEntitySpawn)
		onEntitySpawn(m_entities.back());
}

void EntityManager::tick() {
	// Make a copy so we aren't modifying the vector while iterating over it
	std::vector<std::shared_ptr<Entity>> copy = m_entities;

	// Tick EVERY entity
	for (std::shared_ptr<Entity> entity : copy) {
		// Remove dead m_entities from the system
		if (entity->isDead) {
			entity->world = nullptr;
			m_entities.erase(std::remove(m_entities.begin(), m_entities.end(), entity), m_entities.end());
			auto& container = m_entityContainers[{ entity->bucketPos.x, entity->bucketPos.y }];
			auto& b = container.buckets[entity->bucketPos.z];
			b.m_entities.erase(std::remove_if(b.m_entities.begin(), b.m_entities.end(),
			                                  [&entity](const std::weak_ptr<Entity>& weak) {
				                                  auto locked = weak.lock();
				                                  return !locked ||
				                                         locked == entity; // Remove if expired or matches our entity
			                                  }),
			                   b.m_entities.end());

			if (onEntityDespawn)
				onEntityDespawn(entity);
			continue;
		}
		entity->tick();

		// Check to see if this entity went into another container or bucket
		Int3 newBucketPos = computeBucketPos(entity->posX, entity->posY, entity->posZ);

		if (newBucketPos != entity->bucketPos) {
			// Remove from the old bucket
			auto& oldContainer = m_entityContainers[{ entity->bucketPos.x, entity->bucketPos.y }];
			auto& b = oldContainer.buckets[entity->bucketPos.z];
			b.m_entities.erase(std::remove_if(b.m_entities.begin(), b.m_entities.end(),
			                                  [&entity](const std::weak_ptr<Entity>& weak) {
				                                  auto locked = weak.lock();
				                                  return !locked ||
				                                         locked == entity; // Remove if expired or matches our entity
			                                  }),
			                   b.m_entities.end());

			// Put in the new bucket
			auto& newContainer = m_entityContainers[{ newBucketPos.x, newBucketPos.y }];
			auto& newB = newContainer.buckets[newBucketPos.z];
			newB.m_entities.push_back(entity);
			entity->bucketPos = newBucketPos;
		}
	}
	copy.clear(); // Clear the copy to free memory
}

std::vector<std::shared_ptr<Entity>> EntityManager::getEntitiesWithinAABBExcluding(const AABB& box, EntityId entityId) {
	// Get all entities within an AABB excluding this entity id
	auto entities = getEntitiesWithinAABB(box);
	entities.erase(std::remove_if(entities.begin(), entities.end(),
	                              [entityId](std::shared_ptr<Entity> entity) { return entity->id == entityId; }),
	               entities.end());
	return entities;
}

std::vector<std::shared_ptr<Entity>> EntityManager::getEntitiesWithinAABB(const AABB& box) {
	// Get all m_entities within an AABB
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
			auto& container = m_entityContainers[{ x, z }];
			for (int by = bucketMinY; by <= bucketMaxY; by++) {
				// Get every entity within every bucket
				for (size_t i = 0; i < container.buckets[by].m_entities.size(); i++) {
					// Make sure the weak ptr is still valid
					auto& entityPtrWeak = container.buckets[by].m_entities[i];
					if (auto entityPtrShared = entityPtrWeak.lock()) {
						if (entityPtrShared->collider.intersects(box))
							collidingEntities.push_back(entityPtrShared);
					}
				}
			}
		}
	}
	return collidingEntities;
}

void EntityManager::createEntityFromNBT(Tag& nbt) {
	// Load an entity from the nbt list
	std::string id = nbt.compound["id"].getString();

	// TODO: load other entity types
	if (id == "Item") {
		ItemEntity item({});
		item.loadFromNBT(nbt);
		addEntity(std::make_shared<ItemEntity>(item));
	}
}

std::vector<Tag> EntityManager::collectEntitiesForSave(Int2 cpos, bool clearCollectedEntities) {
	// Collect entities for this chunk coordinates (basically our entity container)
	// We then serialize these entities and return the vector of nbt tags
	// We mark the entities as dead for cleanup afterwards
	std::vector<Tag> collectedEntities;

	auto& container = m_entityContainers[cpos];

	for (int i = 0; i < container.buckets.size(); i++) {
		auto& bucket = container.buckets[i];
		for (auto& entityPtrWeak : bucket.m_entities) {
			// Is this entity dead but not collected?
			if (auto entityPtrShared = entityPtrWeak.lock()) {
				if (entityPtrShared->isDead) continue; // We are dead so no save
				if (entityPtrShared->type == EntityType::PLAYER) continue; // players cannot be saved
				if (clearCollectedEntities) entityPtrShared->isDead = true; // Mark the entity as dead for cleanup
				auto compound = entityPtrShared->serializeToNBT();
				if (!compound)
					continue; // If something went wrong abort save
				collectedEntities.push_back(*compound);
			}
		}
	}
	return collectedEntities;
}

std::optional<std::string> EntityManager::getEntityNbtId(EntityType type) {
	switch (type) {
	case EntityType::ITEM:
		return "Item";
	case EntityType::BOAT:
		return "Boat";
	case EntityType::LIT_TNT:
		return "PrimedTnt";
	case EntityType::ARROW:
		return "Arrow";
	case EntityType::THROWN_SNOWBALL:
		return "Snowball";
	case EntityType::PAINTING:
		return "Painting";
	case EntityType::CREEPER:
		return "Creeper";
	case EntityType::SKELETON:
		return "Skeleton";
	case EntityType::SPIDER:
		return "Spider";
	case EntityType::GIANT_ZOMBIE:
		return "Giant";
	case EntityType::ZOMBIE:
		return "Zombie";
	case EntityType::SLIME:
		return "Slime";
	case EntityType::GHAST:
		return "Ghast";
	case EntityType::ZOMBIE_PIGMAN:
		return "PigZombie";
	case EntityType::PIG:
		return "Pig";
	case EntityType::SHEEP:
		return "Sheep";
	case EntityType::COW:
		return "Cow";
	case EntityType::CHICKEN:
		return "Chicken";
	case EntityType::SQUID:
		return "Squid";
	case EntityType::WOLF:
		return "Wolf";

	// Vanilla only has ONE minecart entity/string
	// There is a type field in the nbt itself
	case EntityType::MINECART:
	case EntityType::STORAGE_MINECART:
	case EntityType::FURNACE_MINECART:
		return "Minecart";

	// Same deal as minecarts
	// not a separate string.
	case EntityType::FALLING_SAND:
	case EntityType::FALLING_GRAVEL:
		return "FallingSand";

	// These have no mapping!
	case EntityType::NONE:
	case EntityType::PLAYER: // Note: Players are saved differently (thanks notch)
	case EntityType::FISH:
	case EntityType::FIREBALL:
	case EntityType::THROWN_EGG:
	case EntityType::FISHING_BOBBER:
	default:
		return std::nullopt;
	}
}