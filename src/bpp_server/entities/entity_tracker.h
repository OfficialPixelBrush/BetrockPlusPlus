/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "../player_conn/player_session.h"
#include "entities.h"
#include "entities/entity_manager.h"
#include "logger.h"
#include "networking/network_stream.h"
#include "networking/packets.h"
#include "world/world.h"
#include <cstdio>
#include <string>

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
	Int3 lastBroadcastPos{};
	int lastBroadcastYaw = 0;
	int lastBroadcastPitch = 0;
	int updateCounter = 0;
	int ticksSinceTeleport = 0;
	std::unordered_set<EntityId> visibleTo; // what player ids can see this entity
};

struct Server;
struct EntityTracker {
	Server* server = nullptr;

	std::unordered_map<EntityId, TrackedEntry> trackedEntities;
	std::unordered_set<EntityId> playerIds;

	void tick();

	void trackEntity(Entity* entity) {
		TrackedEntry entry;
		entry.entity = entity;
		entry.profile = getTrackingProfile(*entity);
		trackedEntities[entity->id] = std::move(entry);
		this->tick();
	}

	void untrackEntity(Entity* entity) {
		this->tick();
	}

	void addPlayer(Entity* player) {
		TrackedEntry entry;
		entry.entity = player;
		entry.profile = getTrackingProfile(*player);
		trackedEntities[player->id] = std::move(entry);
		playerIds.insert(player->id);
		this->tick();
	}

	void removePlayer(Entity* player) {
		this->tick();
	}

	void sendPacketToPlayersInTrackedEntry(Packet::BasePacket& pkt, TrackedEntry& trackedEntry);
	void update(TrackedEntry& trackedEntry);

	// With my strict goal of keeping strict separation we cannot put this as a virtual in the actual entity class itself
	TrackingProfile getTrackingProfile(Entity& entity) {
		const EntityType& type = entity.type;
		switch (type) {
		case EntityType::NONE:
			return { 0, 0, false };
		case EntityType::PLAYER:
			return { 512, 2, false };
		case EntityType::FISH:
			return { 64, 5, true };
		case EntityType::ARROW:
			return { 64, 20, false };
		case EntityType::FIREBALL:
			return { 64, 10, false };
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
			return { 160, 3, false };
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

			GlobalLogger().warn << "EntityTracker: no tracking profile for entity type '" + std::to_string(int(type)) +
			                           "'\n";
		}
	}
};