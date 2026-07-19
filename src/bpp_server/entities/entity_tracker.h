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
	int m_range = 0;
	int m_updateFrequency = 0; // ticks between movement-sync packets
	bool m_sendVelocity = false;
};

struct TrackedEntry {
	Entity* m_entity = nullptr;
	TrackingProfile m_profile{};
	Int32_3 m_lastEncodedPos{};
	Vec3 m_lastBroadcastMotion{};
	int32_t m_lastEncodedYaw = 0;
	int32_t m_lastEncodedPitch = 0;
	int m_updateCounter = 0;
	int m_ticksSinceTeleport = 0;
	std::unordered_set<EntityId> m_visibleTo; // what player ids can see this entity
};

class Server;
struct EntityTracker {
	Server* m_server = nullptr;

	std::unordered_map<EntityId, TrackedEntry> m_trackedEntities;
	std::unordered_set<EntityId> m_playerIds;

	TickTime m_forceTeleportTicks = 400; // 20 seconds

	void tick();

	int16_t quantizeVelocityComponent(double v) {
		v = std::clamp(v, -3.9, 3.9);
		return int16_t(v * 8000.0);
	}
	int32_t quantizePositionComponent(double p) {
		return MathHelper::floor_double(p * 32.0);
	}

	Int16_3 quantizeVelocity(Vec3 v) {
		return { quantizeVelocityComponent(v.m_x), quantizeVelocityComponent(v.m_y), quantizeVelocityComponent(v.m_z) };
	}

	Int32_3 quantizePosition(Vec3 p) {
		return { quantizePositionComponent(p.m_x), quantizePositionComponent(p.m_y), quantizePositionComponent(p.m_z) };
	}

	int32_t quantizeRotation(float r) {
		return MathHelper::floor_float(r * 256.0f / 360.0f);
	}

	void trackEntity(Entity* entity) {
		TrackedEntry entry;
		entry.m_entity = entity;
		entry.m_profile = getTrackingProfile(*entity);

		entry.m_lastEncodedPos = quantizePosition(entity->m_position);
		entry.m_lastBroadcastMotion = entity->m_velocity;
		entry.m_lastEncodedPitch = quantizeRotation(entity->m_rotationPitch);
		entry.m_lastEncodedYaw = quantizeRotation(entity->m_rotationYaw);

		m_trackedEntities[entity->m_id] = std::move(entry);

		// Let any player already in range see this entity right away
		auto& newEntry = m_trackedEntities.at(entity->m_id);
		for (EntityId playerId : m_playerIds) {
			if (playerId == entity->m_id)
				continue;
			auto playerIt = m_trackedEntities.find(playerId);
			if (playerIt == m_trackedEntities.end())
				continue;
			auto& player = playerIt->second;
			auto distanceTo = std::abs(std::max(std::abs(entity->m_position.m_x - player.m_entity->m_position.m_x),
			                                    std::abs(entity->m_position.m_z - player.m_entity->m_position.m_z)));
			if (distanceTo > newEntry.m_profile.m_range)
				continue;
			spawnEntityForPlayer(playerId, newEntry);
		}

		// Force an update
		update(entry);
	}

	void untrackEntity(Entity* entity) {
		auto it = m_trackedEntities.find(entity->m_id);
		if (it == m_trackedEntities.end())
			return;

		despawnEntityForViewers(entity->m_id, it->second);

		for (auto& [id, otherEntry] : m_trackedEntities)
			otherEntry.m_visibleTo.erase(entity->m_id);

		m_trackedEntities.erase(it);
		m_playerIds.erase(entity->m_id);
	}

	void addPlayer(Entity* player) {
		TrackedEntry entry;
		entry.m_entity = player;
		entry.m_profile = getTrackingProfile(*player);

		entry.m_lastEncodedPos = quantizePosition(player->m_position);
		entry.m_lastBroadcastMotion = player->m_velocity;
		entry.m_lastEncodedPitch = quantizeRotation(player->m_rotationPitch);
		entry.m_lastEncodedYaw = quantizeRotation(player->m_rotationYaw);

		m_trackedEntities[player->m_id] = std::move(entry);
		m_playerIds.insert(player->m_id);
		auto& newPlayerEntry = m_trackedEntities.at(player->m_id);

		// This new player should immediately see anything already in range
		for (auto& [entityId, entityEntry] : m_trackedEntities) {
			if (entityId == player->m_id)
				continue;
			auto distanceTo = std::abs(std::max(std::abs(entityEntry.m_entity->m_position.m_x - player->m_position.m_x),
			                                    std::abs(entityEntry.m_entity->m_position.m_z - player->m_position.m_z)));
			if (distanceTo > entityEntry.m_profile.m_range)
				continue;
			spawnEntityForPlayer(player->m_id, entityEntry);
		}

		for (EntityId otherPlayerId : m_playerIds) {
			if (otherPlayerId == player->m_id)
				continue;
			auto otherIt = m_trackedEntities.find(otherPlayerId);
			if (otherIt == m_trackedEntities.end())
				continue;
			auto distanceTo = std::abs(std::max(std::abs(player->m_position.m_x - otherIt->second.m_entity->m_position.m_x),
			                                    std::abs(player->m_position.m_z - otherIt->second.m_entity->m_position.m_z)));
			if (distanceTo > newPlayerEntry.m_profile.m_range)
				continue;
			spawnEntityForPlayer(otherPlayerId, newPlayerEntry);
		}

		// Force an update
		update(entry);
	}

	void removePlayer(Entity* player) {
		untrackEntity(player);
	}

	void spawnEntityForPlayer(EntityId playerId, TrackedEntry& entityEntry);
	void despawnEntityForViewers(EntityId entityId, TrackedEntry& entry);

	void sendPacketToPlayersInTrackedEntry(Packet::BasePacket& pkt, TrackedEntry& trackedEntry);
	void sendPacketToViewers(Packet::BasePacket& pkt, EntityId id);
	TrackedEntry& getTrackerForEntityId(EntityId id);
	void update(TrackedEntry& trackedEntry);

	// With my strict goal of keeping strict separation we cannot put this as a virtual in the actual entity class itself
	TrackingProfile getTrackingProfile(Entity& entity) {
		const EntityType& type = entity.m_type;
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
			return { 160, 20, false };
		case EntityType::PAINTING:
			// Paintings never move so there's nothing to resync
			return { 160, INT_MAX, false };
		default:
			return { 0, 0, false };

			GlobalLogger().m_warn << "EntityTracker: no tracking profile for entity type '" + std::to_string(int(type)) +
			                           "'\n";
		}
	}
};