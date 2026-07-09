/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "entity_tracker.h"
#include "../server.h"
#include "entities.h"
#include "entities/entity_item.h"
#include <algorithm>

static int16_t quantizeVelocity(double v) {
	return MathHelper::floor_double(v * 8000.0);
}
static int32_t quantizePosition(double p) {
	return MathHelper::floor_double(p * 32.0);
}

// Update each player instance so entities properly despawn and spawn for them
void EntityTracker::tick() {
	std::vector<EntityId> deadThisTick;

	for (auto& [entityId, entry] : trackedEntities) {
		if (entry.entity->isDead)
			deadThisTick.push_back(entry.entity->id);
	}

	for (auto& entityId : deadThisTick) {
		auto& entry = trackedEntities.at(entityId);
		for (EntityId viewerId : entry.visibleTo) {
			auto& pSession = server->getSessionById(viewerId);
			Packet::DespawnEntity pkt;
			pkt.entity_id = entityId;
			pkt.Serialize(pSession.stream);
		}
		for (auto& [id, otherEntry] : trackedEntities) {
			otherEntry.visibleTo.erase(entityId);
		}
		trackedEntities.erase(entityId);
		playerIds.erase(entityId);
	}

	// Despawn pass / update
	for (auto& [entityId, entry] : trackedEntities) {
		this->update(entry); // Determine what packets to send
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
			if (distanceTo > entityEntry.profile.range ||
			    entityEntry.visibleTo.find(playerId) != entityEntry.visibleTo.end()) {
				continue;
			}

			auto& pSession = server->getSessionById(playerId);
			switch (entityEntry.entity->type) {
			case EntityType::ITEM: {
				ItemEntity& ie = dynamic_cast<ItemEntity&>(*entityEntry.entity);
				Packet::SpawnItem pkt;
				pkt.entity_id = entityEntry.entity->id;
				pkt.item = ie.itemStack;
				pkt.q_position = { quantizePosition(entityEntry.entity->posX),
					               quantizePosition(entityEntry.entity->posY),
					               quantizePosition(entityEntry.entity->posZ) };
				// For some reason notch decided this should be a convoluted way of getting the initial spawn velocity
				auto quantizeSpawnVelocity = [](double v) -> int8_t {
					return int8_t(v * 128.0);
				};
				pkt.q_rotation = { quantizeSpawnVelocity(entityEntry.entity->motionX),
					               quantizeSpawnVelocity(entityEntry.entity->motionY),
					               quantizeSpawnVelocity(entityEntry.entity->motionZ) };
				pkt.Serialize(pSession.stream);
				break;
			}
			case EntityType::PLAYER: {
				Packet::SpawnPlayer pkt;
				pkt.entity_id = entityEntry.entity->id;
				pkt.held_item_id = ITEM_APPLE;
				pkt.q_position = { quantizePosition(entityEntry.entity->posX),
					               quantizePosition(entityEntry.entity->posY),
					               quantizePosition(entityEntry.entity->posZ) };
				pkt.q_rotation = { 0, 0 };
				pkt.username = server->getUsernameByEntityId(entityEntry.entity->id);
				pkt.Serialize(pSession.stream);
				break;
			}
			default:
				break;
			}
			// TODO: Implement other types
			entityEntry.visibleTo.insert(playerId);
			break;
		}
	}
}

void EntityTracker::sendPacketToPlayersInTrackedEntry(Packet::BasePacket& pkt, TrackedEntry& trackedEntry) {
	for (auto& playerId : trackedEntry.visibleTo) {
		pkt.Serialize(server->getSessionById(playerId).stream);
	}
}

TrackedEntry& EntityTracker::getTrackerForEntityId(EntityId id) {
	for (auto& [entityId, entityEntry] : trackedEntities) {
		if (entityId == id)
			return entityEntry; 
	}
}

void EntityTracker::sendPacketToViewers(Packet::BasePacket& pkt, EntityId id) {
	for (auto& playerId : playerIds) {
		auto& playerEntry = getTrackerForEntityId(playerId);
		if (playerEntry.visibleTo.contains(id)) {
			pkt.Serialize(server->getSessionById(playerId).stream);
		}
	}
}

void EntityTracker::update(TrackedEntry& trackedEntry) {
	auto& entity = trackedEntry.entity;
	Int3 currentPosition = { quantizePosition(entity->posX), quantizePosition(entity->posY),
		                     quantizePosition(entity->posZ) };

	// Check the magnitude of the velocity and if it has changed since the last tick
	Vec3 currentMotion = { entity->motionX, entity->motionY, entity->motionZ };
	Vec3& lastMotion = trackedEntry.lastBroadcastMotion;
	double currentMagnitude = std::sqrt(currentMotion.x * currentMotion.x + currentMotion.y * currentMotion.y + currentMotion.z * currentMotion.z);
	double lastMagnitude = std::sqrt(lastMotion.x * lastMotion.x + lastMotion.y * lastMotion.y + lastMotion.z * lastMotion.z);

	// If our velocityChanged flag is dirty force a velocity update independent if the tracker profile allows it
	// Else, check to see if the difference in magnitude for the current and last motion vectors are past a threshold
	bool needsVelocityUpdate = entity->velocityChanged || (trackedEntry.profile.sendVelocity && std::abs(currentMagnitude - lastMagnitude) >= 0.001);

	// Send velocity
	if (needsVelocityUpdate) {
		entity->velocityChanged = false;
		Packet::EntityVelocity pkt;
		pkt.entity_id = entity->id;
		pkt.velocity = { quantizeVelocity(entity->motionX), quantizeVelocity(entity->motionY), quantizeVelocity(entity->motionZ) }; // Quantized for 16 bit shorts
		sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
	}

	trackedEntry.ticksSinceTeleport++;
	trackedEntry.updateCounter++;

	bool needsMovementUpdate = trackedEntry.updateCounter >= trackedEntry.profile.updateFrequency || trackedEntry.ticksSinceTeleport >= 40; // Force a resync

	// Reset our values
	if (needsMovementUpdate) {
		trackedEntry.updateCounter = 0;

		// Check our distance from our last broadcast position
		auto xdist = currentPosition.x - trackedEntry.lastBroadcastPos.x;
		auto ydist = currentPosition.y - trackedEntry.lastBroadcastPos.y;
		auto zdist = currentPosition.z - trackedEntry.lastBroadcastPos.z;
		auto dist = std::sqrt((xdist * xdist) + (ydist * ydist) + (zdist * zdist));

		bool needsTP = dist >= double(quantizePosition(4)) || trackedEntry.ticksSinceTeleport >= forceTeleportTicks;

		if (needsTP) {
			trackedEntry.ticksSinceTeleport = 0;
			Packet::TeleportEntity pkt;
			pkt.entity_id = entity->id;
			pkt.position = { currentPosition.x, MathHelper::floor_double(currentPosition.y - (1.0 / 64.0)), currentPosition.z };
			pkt.rotation = { int8_t((entity->rotationYaw / 360.0) * 256.0),
				             int8_t((entity->rotationPitch / 360.0) * 256.0) };
			sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
			trackedEntry.lastBroadcastPos = currentPosition;
			trackedEntry.lastBroadcastYaw = entity->rotationYaw;
			trackedEntry.lastBroadcastPitch = entity->rotationPitch;
		} else {
			bool needsRelMove = dist > 0;
			bool needsRot = (trackedEntry.lastBroadcastYaw != entity->rotationYaw) ||
			                (trackedEntry.lastBroadcastPitch != entity->rotationPitch);

			if (needsRelMove && needsRot) {
				Packet::EntityPositionAndRotation pkt;
				pkt.qr_position = { int8_t(xdist), int8_t(ydist), int8_t(zdist) };
				pkt.q_rotation = { int8_t((entity->rotationYaw / 360.0) * 256.0),
					               int8_t((entity->rotationPitch / 360.0) * 256.0) };
				pkt.entity_id = entity->id;
				sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
				trackedEntry.lastBroadcastPos = currentPosition;
				trackedEntry.lastBroadcastYaw = entity->rotationYaw;
				trackedEntry.lastBroadcastPitch = entity->rotationPitch;
				return;
			};
			if (needsRelMove) {
				Packet::EntityPosition pkt;
				pkt.qr_position = { int8_t(xdist), int8_t(ydist), int8_t(zdist) };
				pkt.entity_id = entity->id;
				sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
				trackedEntry.lastBroadcastPos = currentPosition;
				return;
			}
			if (needsRot) {
				Packet::EntityRotation pkt;
				pkt.q_rotation = { int8_t((entity->rotationYaw / 360.0) * 256.0),
					               int8_t((entity->rotationPitch / 360.0) * 256.0) };
				pkt.entity_id = entity->id;
				sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
				trackedEntry.lastBroadcastYaw = entity->rotationYaw;
				trackedEntry.lastBroadcastPitch = entity->rotationPitch;
				return;
			}
		}
	}
}