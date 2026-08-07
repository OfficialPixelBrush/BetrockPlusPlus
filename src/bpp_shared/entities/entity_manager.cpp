/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "entity_manager.h"
#include "entity_item.h"
#include "entity_chicken.h"
#include "entity_cow.h"
#include "entity_pig.h"
#include "entity_sheep.h"
#include "world.h"

void EntityManager::RemoveEntity(EntityId _id) {
	// Find the entity for this ID
	auto it = std::find_if(entities.begin(), entities.end(),
	                       [_id](const std::shared_ptr<Entity>& _e) { return _e->id == _id; });
	if (it == entities.end()) {
		return; // Not found, nothing to do
	}

	std::shared_ptr<Entity> entity = *it;

	// Remove from its bucket (if the container still exists — unload may have erased it)
	Int2 cpos{ entity->bucketPos.x, entity->bucketPos.y };
	auto containerIt = entityContainers.find(cpos);
	if (containerIt != entityContainers.end()) {
		auto& bucket = containerIt->second.buckets[entity->bucketPos.z];
		bucket.entities.erase(std::remove_if(bucket.entities.begin(), bucket.entities.end(),
		                                     [&entity](const std::weak_ptr<Entity>& _weak) {
			                                     auto locked = _weak.lock();
			                                     return !locked || locked == entity;
		                                     }),
		                      bucket.entities.end());
		PruneEmptyContainer(cpos);
	}

	// Remove from the master list
	entities.erase(it);

	// Unbind ourselves
	entity->entityManager = nullptr;

	// Mark the chunk we died in as dirty
	if (auto chunk = world->GetChunk({ entity->bucketPos.x, entity->bucketPos.y }))
		chunk->isModified = true;

	// Set as dead for cleanup
	entity->isDead = true;
	if (onEntityDespawn)
		onEntityDespawn(entity);
}

void EntityManager::AddEntity(std::shared_ptr<Entity> _entity, EntityId _forceEntityId) {
	if (!world) {
		GlobalLogger().error << "Attempted to add an entity before EntityManager was bound to a world!\n";
		return;
	}
	_entity->id = _forceEntityId == -1 ? GetNextEntityId()
	                                   : _forceEntityId; // Assign an ID if we weren't forced to use one
	_entity->world = world; // Bind the world pointer so the entity can interact with the world
	_entity->entityManager = this;
	_entity->dim = (Dimension)world->GetDimension();

	// Register the entity into its initial bucket
	_entity->bucketPos = ComputeBucketPos(_entity->position);
	auto& container = entityContainers[{ _entity->bucketPos.x, _entity->bucketPos.y }];
	container.buckets[_entity->bucketPos.z].entities.push_back(_entity);

	// If we are a player register for easy lookup
	if (_entity->type == EntityType::PLAYER) {
		std::weak_ptr<Entity> weakEntity = _entity;
		players.push_back(weakEntity);
	}

	entities.push_back(std::move(_entity));
	if (onEntitySpawn)
		onEntitySpawn(entities.back());
}

void EntityManager::Tick() {
	// Snapshot weak refs so mid-tick spawn/despawn cannot invalidate iteration,
	// without the full shared_ptr vector copy every tick.
	std::vector<std::weak_ptr<Entity>> toTick;
	toTick.reserve(entities.size());
	for (const auto& entity : entities)
		toTick.emplace_back(entity);

	for (const auto& weak : toTick) {
		std::shared_ptr<Entity> entity = weak.lock();
		if (!entity || entity->isDead)
			continue;

		entity->Tick();

		// Check to see if this entity went into another container or bucket
		Int3 newBucketPos = ComputeBucketPos(entity->position);

		if (newBucketPos != entity->bucketPos) {
			Int2 oldCpos{ entity->bucketPos.x, entity->bucketPos.y };
			// Remove from the old bucket and mark the old chunk as modified
			if (auto chunk = world->GetChunk(oldCpos))
				chunk->isModified = true;
			auto& oldContainer = entityContainers[oldCpos];
			auto& b = oldContainer.buckets[entity->bucketPos.z];
			b.entities.erase(std::remove_if(b.entities.begin(), b.entities.end(),
			                                [&entity](const std::weak_ptr<Entity>& _weak) {
				                                auto locked = _weak.lock();
				                                return !locked ||
				                                       locked == entity; // Remove if expired or matches our entity
			                                }),
			                 b.entities.end());
			PruneEmptyContainer(oldCpos);

			// Put in the new bucket
			auto& newContainer = entityContainers[{ newBucketPos.x, newBucketPos.y }];
			auto& newB = newContainer.buckets[newBucketPos.z];
			newB.entities.push_back(entity);
			entity->bucketPos = newBucketPos;
		}
	}

	// Remove dead entities after ticking so iteration stays stable
	for (size_t i = 0; i < entities.size();) {
		if (entities[i]->isDead)
			RemoveEntity(entities[i]->id);
		else
			++i;
	}

	// Clear players that have expired
	for (auto it = players.begin(); it != players.end();) {
		if (it->expired()) {
			it = players.erase(it);
		} else {
			++it;
		}
	}
}

std::vector<std::shared_ptr<Entity>> EntityManager::GetEntitiesWithinAabbExcluding(const AABB& _box,
                                                                                   const EntityId _entityId) {
	// Get all entities within an AABB excluding this entity id
	auto entitiesInAABB = GetEntitiesWithinAabb(_box);
	entitiesInAABB.erase(std::remove_if(entitiesInAABB.begin(), entitiesInAABB.end(),
	                                    [_entityId](std::shared_ptr<Entity> _entity) {
		                                    return _entity->id == _entityId;
	                                    }),
	                     entitiesInAABB.end());
	return entitiesInAABB;
}

std::vector<std::shared_ptr<Entity>> EntityManager::GetEntitiesWithinAabbExcludingTypes(
    const AABB& _box, const std::vector<EntityType>& _excludedTypes) {
	std::vector<std::shared_ptr<Entity>> exclusiveEntities;
	auto entitiesInAABB = GetEntitiesWithinAabb(_box);
	for (auto& entity : entitiesInAABB) {
		if (std::find(_excludedTypes.begin(), _excludedTypes.end(), entity->type) == _excludedTypes.end()) {
			exclusiveEntities.push_back(entity);
		}
	}
	return exclusiveEntities;
}

std::vector<std::shared_ptr<Entity>> EntityManager::GetEntitiesWithinAabb(const AABB& _box) {
	// Get all entities within an AABB
	std::vector<std::shared_ptr<Entity>> collidingEntities;

	// Normalize to block coordinates
	int blockMinX = MathHelper::FloorDouble((_box.minX - 2.0) / 16.0);
	int blockMinZ = MathHelper::FloorDouble((_box.minZ - 2.0) / 16.0);
	int blockMaxX = MathHelper::FloorDouble((_box.maxX + 2.0) / 16.0);
	int blockMaxZ = MathHelper::FloorDouble((_box.maxZ + 2.0) / 16.0);

	// Get our start and end bucket
	int bucketMinY = MathHelper::FloorDouble((_box.minY - 2.0) / 16.0);
	int bucketMaxY = MathHelper::FloorDouble((_box.maxY + 2.0) / 16.0);
	bucketMinY = std::max(0, bucketMinY);
	bucketMinY = std::min(9, bucketMinY);
	bucketMaxY = std::max(0, bucketMaxY);
	bucketMaxY = std::min(9, bucketMaxY);

	// Go through each block position (find-only: never insert empty containers)
	for (int x = blockMinX; x <= blockMaxX; x++) {
		for (int z = blockMinZ; z <= blockMaxZ; z++) {
			auto it = entityContainers.find({ x, z });
			if (it == entityContainers.end())
				continue;
			auto& container = it->second;
			for (int by = bucketMinY; by <= bucketMaxY; by++) {
				// Get every entity within every bucket
				for (size_t i = 0; i < container.buckets[by].entities.size(); i++) {
					// Make sure the weak ptr is still valid
					auto& entityPtrWeak = container.buckets[by].entities[i];
					if (auto entityPtrShared = entityPtrWeak.lock()) {
						if (entityPtrShared->collider.Intersects(_box))
							collidingEntities.push_back(entityPtrShared);
					}
				}
			}
		}
	}
	return collidingEntities;
}

void EntityManager::CreateEntityFromNbt(Tag& _nbt) {
	// Load an entity from the nbt list
	std::string id = _nbt.compound["id"].GetString();

	// TODO: load other entity types
	if (id == "Item") {
		ItemEntity entity({});
		entity.LoadFromNbt(_nbt);
		AddEntity(std::make_shared<ItemEntity>(entity));
	}
	if (id == "Cow") {
		CowEntity entity;
		entity.LoadFromNbt(_nbt);
		AddEntity(std::make_shared<CowEntity>(entity));
	}
	if (id == "Pig") {
		PigEntity entity;
		entity.LoadFromNbt(_nbt);
		AddEntity(std::make_shared<PigEntity>(entity));
	}
	if (id == "Sheep") {
		SheepEntity entity;
		entity.LoadFromNbt(_nbt);
		AddEntity(std::make_shared<SheepEntity>(entity));
	}
	if (id == "Chicken") {
		ChickenEntity entity;
		entity.LoadFromNbt(_nbt);
		AddEntity(std::make_shared<ChickenEntity>(entity));
	}
}

std::vector<Tag> EntityManager::CollectEntitiesForSave(Int2 _cpos, bool _clearCollectedEntities) {
	// Collect entities for this chunk coordinates (basically our entity container)
	// We then serialize these entities and return the vector of nbt tags
	// We mark the entities as dead for cleanup afterwards
	std::vector<Tag> collectedEntities;

	auto it = entityContainers.find(_cpos);
	if (it == entityContainers.end())
		return collectedEntities;

	auto& container = it->second;
	for (size_t i = 0; i < container.buckets.size(); i++) {
		auto& bucket = container.buckets[i];
		for (auto& entityPtrWeak : bucket.entities) {
			// Is this entity dead but not collected?
			if (auto entityPtrShared = entityPtrWeak.lock()) {
				if (entityPtrShared->isDead)
					continue; // We are dead so no save
				if (entityPtrShared->type == EntityType::PLAYER)
					continue; // players cannot be saved
				if (_clearCollectedEntities)
					entityPtrShared->isDead = true; // Mark the entity as dead for cleanup
				auto compound = entityPtrShared->SerializeToNbt();
				if (!compound)
					continue; // If something went wrong abort save
				collectedEntities.push_back(*compound);
			}
		}
	}

	if (_clearCollectedEntities)
		EraseContainer(_cpos);
	else
		PruneEmptyContainer(_cpos);

	return collectedEntities;
}

std::optional<std::string> EntityManager::GetEntityNbtId(EntityType _type) {
	switch (_type) {
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