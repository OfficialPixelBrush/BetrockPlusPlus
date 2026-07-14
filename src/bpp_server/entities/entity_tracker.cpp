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
				pkt.held_item_id = Items::Id::NONE;
				pkt.q_position = { quantizePosition(entityEntry.entity->posX),
					               quantizePosition(entityEntry.entity->posY),
					               quantizePosition(entityEntry.entity->posZ) };
				pkt.q_rotation = { int8_t(quantizeRotation(entityEntry.entity->rotationYaw)),
					               int8_t(quantizeRotation(entityEntry.entity->rotationPitch)) };

				// To prevent bad behavior when we share a name with another entity
				auto username = server->getUsernameByEntityId(entityEntry.entity->id);
				pkt.username = username;
				pkt.Serialize(pSession.stream);
				break;
			}
			default:
				break;
			}
			// TODO: Implement other types
			entityEntry.visibleTo.insert(playerId);
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
	Vec3 currentPosition = { entity->posX, entity->posY, entity->posZ };

	// Dirty flag gets checked every tick
	if (entity->velocityChanged) {
		entity->velocityChanged = false;
		trackedEntry.lastBroadcastMotion = { entity->motionX, entity->motionY, entity->motionZ };
		Packet::EntityVelocity pkt;
		pkt.entity_id = entity->id;
		pkt.velocity = { quantizeVelocity(entity->motionX), quantizeVelocity(entity->motionY),
			             quantizeVelocity(entity->motionZ) };
		sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
	}

	trackedEntry.ticksSinceTeleport++;
	trackedEntry.updateCounter++;

	bool needsMovementUpdate = trackedEntry.updateCounter >= trackedEntry.profile.updateFrequency ||
	                           trackedEntry.ticksSinceTeleport >= forceTeleportTicks;

	if (needsMovementUpdate) {
		trackedEntry.updateCounter = 0;

		// The threshold-based velocity check
		if (trackedEntry.profile.sendVelocity) {
			Vec3 currentMotion = { entity->motionX, entity->motionY, entity->motionZ };
			Vec3& lastMotion = trackedEntry.lastBroadcastMotion;
			double dmx = currentMotion.x - lastMotion.x;
			double dmy = currentMotion.y - lastMotion.y;
			double dmz = currentMotion.z - lastMotion.z;
			double deltaSq = dmx * dmx + dmy * dmy + dmz * dmz;
			const double motionThreshold = 0.02;

			bool needsVelocityUpdate = deltaSq > motionThreshold * motionThreshold ||
			                           (deltaSq > 0.0 && currentMotion.x == 0.0 && currentMotion.y == 0.0 &&
			                            currentMotion.z == 0.0);

			if (needsVelocityUpdate) {
				lastMotion = currentMotion;
				Packet::EntityVelocity pkt;
				pkt.entity_id = entity->id;
				pkt.velocity = { quantizeVelocity(entity->motionX), quantizeVelocity(entity->motionY),
					             quantizeVelocity(entity->motionZ) };
				sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
			}
		}

		int32_t qx = quantizePosition(currentPosition.x);
		int32_t qy = quantizePosition(currentPosition.y);
		int32_t qz = quantizePosition(currentPosition.z);
		int32_t qYaw = quantizeRotation(entity->rotationYaw);
		int32_t qPitch = quantizeRotation(entity->rotationPitch);

		int32_t dx = qx - trackedEntry.lastEncodedPos.x;
		int32_t dy = qy - trackedEntry.lastEncodedPos.y;
		int32_t dz = qz - trackedEntry.lastEncodedPos.z;

		bool needsTP = dx < -128 || dx >= 128 || dy < -128 || dy >= 128 || dz < -128 || dz >= 128 ||
		               trackedEntry.ticksSinceTeleport >= forceTeleportTicks;

		if (needsTP) {
			trackedEntry.ticksSinceTeleport = 0;

			// resyncs the entity position
			entity->posX = double(qx) / 32.0;
			entity->posY = double(qy) / 32.0;
			entity->posZ = double(qz) / 32.0;
			entity->rebuildCollider();

			Packet::TeleportEntity pkt;
			pkt.entity_id = entity->id;
			pkt.position = { qx, qy, qz };
			pkt.rotation = { int8_t(qYaw), int8_t(qPitch) };
			sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
			trackedEntry.lastEncodedPos = { qx, qy, qz };
			trackedEntry.lastEncodedYaw = qYaw;
			trackedEntry.lastEncodedPitch = qPitch;
		} else {
			bool needsRelMove = dx != 0 || dy != 0 || dz != 0;
			bool needsRot = qYaw != trackedEntry.lastEncodedYaw || qPitch != trackedEntry.lastEncodedPitch;

			if (needsRelMove && needsRot) {
				Packet::EntityPositionAndRotation pkt;
				pkt.qr_position = { int8_t(dx), int8_t(dy), int8_t(dz) };
				pkt.q_rotation = { int8_t(qYaw), int8_t(qPitch) };
				pkt.entity_id = entity->id;
				sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
				trackedEntry.lastEncodedPos = { qx, qy, qz };
				trackedEntry.lastEncodedYaw = qYaw;
				trackedEntry.lastEncodedPitch = qPitch;
				return;
			};
			if (needsRelMove) {
				Packet::EntityPosition pkt;
				pkt.qr_position = { int8_t(dx), int8_t(dy), int8_t(dz) };
				pkt.entity_id = entity->id;
				sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
				trackedEntry.lastEncodedPos = { qx, qy, qz };
				return;
			}
			if (needsRot) {
				Packet::EntityRotation pkt;
				pkt.q_rotation = { int8_t(qYaw), int8_t(qPitch) };
				pkt.entity_id = entity->id;
				sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
				trackedEntry.lastEncodedYaw = qYaw;
				trackedEntry.lastEncodedPitch = qPitch;
				return;
			}
		}
	}
}