/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "entity_tracker.h"
#include "../server.h"

// Update each player instance so entities properly despawn and spawn for them
void EntityTracker::tick() {
	// Despawn pass
	for (auto& [entityId, entry] : trackedEntities) {
		for (auto it = entry.visibleTo.begin(); it != entry.visibleTo.end();) {
			EntityId playerId = *it;
			auto playerIt = trackedEntities.find(playerId);
			if (playerIt == trackedEntities.end()) {
				it = entry.visibleTo.erase(it);
				continue;
			}
			auto& player = playerIt->second;

			auto distanceTo = std::abs(std::max(std::abs(entry.entity->posX - player.entity->posX),
			                                    std::abs(entry.entity->posZ - player.entity->posZ)));
			if (distanceTo > entry.profile.range) {
				auto& pSession = server->getSessionById(playerId);
				Packet::DespawnEntity pkt;
				pkt.entity_id = entry.entity->id;
				pkt.Serialize(pSession.stream);

				it = entry.visibleTo.erase(it);
				GlobalLogger().info << "Despawning entity " << entry.entity->id << " for player " << playerId << "\n";
			} else {
				++it;
			}
		}
	}

	// Spawn pass
	for (EntityId playerId : playerIds) {
		auto& player = trackedEntities.at(playerId);

		for (auto& [entityId, entityEntry] : trackedEntities) {
			if (entityId == playerId)
				continue;

			auto distanceTo = std::abs(std::max(std::abs(entityEntry.entity->posX - player.entity->posX),
			                                    std::abs(entityEntry.entity->posZ - player.entity->posZ)));
			if (distanceTo <= entityEntry.profile.range &&
			    entityEntry.visibleTo.find(playerId) == entityEntry.visibleTo.end()) {
				auto& pSession = server->getSessionById(playerId);
				if (entityEntry.entity->type == "Item") {
					Packet::SpawnItem pkt;
					pkt.entity_id = entityEntry.entity->id;
					pkt.item = { 4, 1, 0 };
					pkt.q_pitch = entityEntry.entity->rotationPitch;
					pkt.q_yaw = entityEntry.entity->rotationYaw;
					pkt.q_position = { int32_t(entityEntry.entity->posX * 32.0),
						               int32_t(entityEntry.entity->posY * 32.0),
						               int32_t(entityEntry.entity->posZ * 32.0) };
					pkt.Serialize(pSession.stream);
				}
				// TODO: SpawnPlayer / SpawnMob / SpawnObject / SpawnPainting for other types

				entityEntry.visibleTo.insert(playerId);
				GlobalLogger().info << "Spawning entity " << entityEntry.entity->id << " for player " << playerId
				                    << "\n";
			}
		}
	}
}