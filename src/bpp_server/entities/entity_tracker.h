/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "../player_conn/player_session.h"
#include "entities/entity_manager.h"
#include "logger.h"
#include "world/world.h"
#include "networking/network_stream.h"
#include "networking/packets.h"
#include <cstdio>

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
		trackedEntities.erase(entity->id);
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
		for (auto& [id, entry] : trackedEntities) {
			entry.visibleTo.erase(player->id);
		}
		playerIds.erase(player->id);
		trackedEntities.erase(player->id);
		this->tick();
	}

	// With my strict goal of keeping strict separation we cannot put this as a virtual in the actual entity class itself
	TrackingProfile getTrackingProfile(Entity& entity) {
		const std::string& type = entity.type;
		if (type == "Player") {
			return { 512, 2, false };
		} else if (type == "Fish") {
			return { 64, 5, true };
		} else if (type == "Arrow") {
			return { 64, 20, false };
		} else if (type == "Fireball") {
			return { 64, 10, false };
		} else if (type == "Snowball") {
			return { 64, 10, true };
		} else if (type == "Egg") {
			return { 64, 10, true };
		} else if (type == "Item") {
			return { 64, 20, true };
		} else if (type == "Minecart") {
			return { 160, 5, true };
		} else if (type == "Boat") {
			return { 160, 5, true };
		} else if (type == "Squid") {
			return { 160, 3, true };
		} else if (isGenericMob(type)) {
			// Fallback for every plain land mob
			return { 160, 3, false };
		} else if (type == "TNTPrimed") {
			return { 160, 10, true };
		} else if (type == "FallingSand") {
			return { 160, 20, true };
		} else if (type == "Painting") {
			// Paintings never move so there's nothing to resync
			return { 160, INT_MAX, false };
		}

		GlobalLogger().warn << "EntityTracker: no tracking profile for entity type '" + type + "'\n";
	}

private:
	bool isGenericMob(const std::string& type) const {
		static const std::unordered_set<std::string> genericMobs = { "Chicken",    "Cow",    "Pig",       "Sheep",
			                                                         "Wolf",       "Zombie", "PigZombie", "Skeleton",
			                                                         "Creeper",    "Spider", "Ghast",     "Slime",
			                                                         "GiantZombie" };
		return genericMobs.count(type) != 0;
	}
};