/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include "../player_conn/player_session.h"
#include "entities/entity_manager.h"
#include "logger.h"
#include "world/world.h"
#include <cstdio>

// Entity tracker so we can send entity updates to the right players. This is server side only annoyingly enough.
// I am not entirely happy with how this is done but notch demands we have several packet types for each type of entity
struct TrackingProfile {
	int range;
	int updateFrequency; // ticks between movement-sync packets
	bool sendVelocity;
};

struct TrackedEntry {
	Entity* entity;
	TrackingProfile profile;
	Vec3 lastBroadcastPos{};
	int lastBroadcastYaw = 0;
	int lastBroadcastPitch = 0;
	int updateCounter = 0;
	int ticksSinceTeleport = 0;
	std::unordered_set<int64_t> visibleTo; // what player ids can see this entity
};

struct EntityTracker {
	std::vector<TrackedEntry> trackedEntities;
	std::vector<TrackedEntry> trackedPlayers;

	void tick();
	void trackEntity(Entity* entity);
	void untrackEntity(Entity* entity);
	void addPlayer(PlayerSession* player) {
		TrackedEntry playerEntry;
		playerEntry.entity = player->entity.get();
		playerEntry.profile = getTrackingProfile(*player->entity);
		trackedPlayers.push_back(std::move(playerEntry));
	}

	// Remove the player from the trackedPlayers list and remove them from all visibleTo sets
	void removePlayer(PlayerSession* player) {
		for (auto& entry : trackedEntities) {
			entry.visibleTo.erase(player->entityId);
		}
		trackedPlayers.erase(std::remove_if(trackedPlayers.begin(), trackedPlayers.end(),
		                                    [player](const TrackedEntry& entry) {
			                                    return entry.entity->id == player->entityId;
		                                    }),
		                     trackedPlayers.end());
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