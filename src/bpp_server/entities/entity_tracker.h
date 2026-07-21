/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "entities.h"
#include "entities/entity_manager.h"
#include "logger.h"
#include "networking/network_stream.h"
#include "networking/packets.h"
#include "world/world.h"
#include <cstdio>
#include <string>
#include <unordered_map>
#include <unordered_set>

// Entity tracker so we can send entity updates to the right players. This is server side only annoyingly enough.
// I am not entirely happy with how this is done but notch demands we have several packet types for each type of entity
struct TrackingProfile {
	int range = 0;
	int updateFrequency = 0; // ticks between movement-sync packets
	bool sendVelocity = false;
};

struct TrackedEntry {
	Entity* entity = nullptr;
	TrackingProfile profile{};
	Int32_3 lastEncodedPos{};
	Vec3 lastBroadcastMotion{};
	int32_t lastEncodedYaw = 0;
	int32_t lastEncodedPitch = 0;
	int updateCounter = 0;
	int ticksSinceTeleport = 0;
	std::unordered_set<EntityId> visibleTo; // what player ids can see this entity
};

class Server;
struct EntityTracker {
	Server* server = nullptr;

	std::unordered_map<EntityId, TrackedEntry> trackedEntities;
	std::unordered_set<EntityId> playerIds;

	TickTime forceTeleportTicks = 400; // 20 seconds

	void Tick();

	int16_t QuantizeVelocityComponent(double _v) {
		_v = std::clamp(_v, -3.9, 3.9);
		return int16_t(_v * 8000.0);
	}
	int32_t QuantizePositionComponent(double _p) {
		return MathHelper::FloorDouble(_p * 32.0);
	}

	Int16_3 QuantizeVelocity(Vec3 _v) {
		return { QuantizeVelocityComponent(_v.x), QuantizeVelocityComponent(_v.y), QuantizeVelocityComponent(_v.z) };
	}

	Int32_3 QuantizePosition(Vec3 _p) {
		return { QuantizePositionComponent(_p.x), QuantizePositionComponent(_p.y), QuantizePositionComponent(_p.z) };
	}

	int32_t QuantizeRotation(float _r) {
		return MathHelper::FloorFloat(_r * 256.0f / 360.0f);
	}

	void TrackEntity(Entity* _entity) {
		TrackedEntry entry;
		entry.entity = _entity;
		entry.profile = GetTrackingProfile(*_entity);

		entry.lastEncodedPos = QuantizePosition(_entity->position);
		entry.lastBroadcastMotion = _entity->velocity;
		entry.lastEncodedPitch = QuantizeRotation(_entity->rotationPitch);
		entry.lastEncodedYaw = QuantizeRotation(_entity->rotationYaw);

		trackedEntities[_entity->id] = std::move(entry);

		// Let any player already in range see this entity right away
		auto& newEntry = trackedEntities.at(_entity->id);
		for (EntityId playerId : playerIds) {
			if (playerId == _entity->id)
				continue;
			auto playerIt = trackedEntities.find(playerId);
			if (playerIt == trackedEntities.end())
				continue;
			auto& player = playerIt->second;
			auto distanceTo = std::abs(std::max(std::abs(_entity->position.x - player.entity->position.x),
			                                    std::abs(_entity->position.z - player.entity->position.z)));
			if (distanceTo > newEntry.profile.range)
				continue;
			SpawnEntityForPlayer(playerId, newEntry);
		}

		// Force an update
		Update(entry);
	}

	void UntrackEntity(Entity* _entity) {
		auto it = trackedEntities.find(_entity->id);
		if (it == trackedEntities.end())
			return;

		DespawnEntityForViewers(_entity->id, it->second);

		for (auto& [id, otherEntry] : trackedEntities)
			otherEntry.visibleTo.erase(_entity->id);

		trackedEntities.erase(it);
		playerIds.erase(_entity->id);
	}

	void AddPlayer(Entity* _player) {
		TrackedEntry entry;
		entry.entity = _player;
		entry.profile = GetTrackingProfile(*_player);

		entry.lastEncodedPos = QuantizePosition(_player->position);
		entry.lastBroadcastMotion = _player->velocity;
		entry.lastEncodedPitch = QuantizeRotation(_player->rotationPitch);
		entry.lastEncodedYaw = QuantizeRotation(_player->rotationYaw);

		trackedEntities[_player->id] = std::move(entry);
		playerIds.insert(_player->id);
		auto& newPlayerEntry = trackedEntities.at(_player->id);

		// This new player should immediately see anything already in range
		for (auto& [entityId, entityEntry] : trackedEntities) {
			if (entityId == _player->id)
				continue;
			auto distanceTo = std::abs(std::max(std::abs(entityEntry.entity->position.x - _player->position.x),
			                                    std::abs(entityEntry.entity->position.z - _player->position.z)));
			if (distanceTo > entityEntry.profile.range)
				continue;
			SpawnEntityForPlayer(_player->id, entityEntry);
			entityEntry.visibleTo.insert(_player->id);
		}

		for (EntityId otherPlayerId : playerIds) {
			if (otherPlayerId == _player->id)
				continue;
			auto otherIt = trackedEntities.find(otherPlayerId);
			if (otherIt == trackedEntities.end())
				continue;
			auto distanceTo = std::abs(std::max(std::abs(_player->position.x - otherIt->second.entity->position.x),
			                                    std::abs(_player->position.z - otherIt->second.entity->position.z)));
			if (distanceTo > newPlayerEntry.profile.range)
				continue;
			SpawnEntityForPlayer(otherPlayerId, newPlayerEntry);
			newPlayerEntry.visibleTo.insert(otherPlayerId);
		}

		// Force an update
		Update(newPlayerEntry);
	}

	void RemovePlayer(Entity* _player) {
		UntrackEntity(_player);
	}

	void SpawnEntityForPlayer(EntityId _playerId, TrackedEntry& _entityEntry);
	void DespawnEntityForViewers(EntityId _entityId, TrackedEntry& _entry);

	void SendPacketToPlayersInTrackedEntry(Packet::BasePacket& _pkt, TrackedEntry& _trackedEntry);
	void SendPacketToViewers(Packet::BasePacket& _pkt, EntityId _id);
	TrackedEntry& GetTrackerForEntityId(EntityId _id);
	void Update(TrackedEntry& _trackedEntry);

	// With my strict goal of keeping strict separation we cannot put this as a virtual in the actual entity class itself
	TrackingProfile GetTrackingProfile(Entity& _entity) {
		const EntityType& type = _entity.type;
		switch (type) {
		case EntityType::NONE:
			return { 0, 0, false };
		case EntityType::PLAYER:
			return { 512, 2, false };
		case EntityType::FISH:
			return { 64, 5, true };
		case EntityType::ARROW:
			return { 64, 20, true };
		case EntityType::FIREBALL:
			return { 64, 10, true };
		case EntityType::THROWN_SNOWBALL:
		case EntityType::THROWN_EGG:
			return { 64, 10, true };
		case EntityType::ITEM:
			return { 64, 20, true };
		case EntityType::MINECART:
		case EntityType::BOAT:
			return { 160, 5, true };
		case EntityType::SQUID:
			return { 160, 3, true };
		case EntityType::CHICKEN:
		case EntityType::COW:
		case EntityType::PIG:
		case EntityType::SHEEP:
		case EntityType::WOLF:
		case EntityType::ZOMBIE:
		case EntityType::ZOMBIE_PIGMAN:
		case EntityType::SKELETON:
		case EntityType::CREEPER:
		case EntityType::SPIDER:
		case EntityType::GHAST:
		case EntityType::SLIME:
		case EntityType::GIANT_ZOMBIE:
			return { 160, 3, true };
		case EntityType::LIT_TNT:
			return { 160, 10, true };
		case EntityType::FALLING_SAND:
		case EntityType::FALLING_GRAVEL:
			return { 160, 20, true };
		case EntityType::PAINTING:
			// Paintings never move so there's nothing to resync
			return { 160, INT_MAX, false };
		default:
			return { 0, 0, false };

			GlobalLogger().warn << "EntityTracker: no tracking profile for entity type '" + std::to_string(int(type)) + "'\n";
		}
	}
};