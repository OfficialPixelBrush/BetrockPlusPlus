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
#include "packet_data.h"
#include <algorithm>
#include <cstdint>

// Update each player instance so entities properly despawn and spawn for them
void EntityTracker::tick() {
	std::vector<EntityId> deadThisTick;

	for (auto& [entityId, entry] : m_trackedEntities) {
		if (entry.m_entity->m_isDead)
			deadThisTick.push_back(entry.m_entity->m_id);
	}

	for (auto& entityId : deadThisTick) {
		auto& entry = m_trackedEntities.at(entityId);
		despawnEntityForViewers(entityId, entry);
		for (auto& [id, otherEntry] : m_trackedEntities) {
			otherEntry.m_visibleTo.erase(entityId);
		}
		m_trackedEntities.erase(entityId);
		m_playerIds.erase(entityId);
		GlobalLogger().m_info << "Killed entity id " << entityId << "\n";
	}

	// Despawn pass / update
	for (auto& [entityId, entry] : m_trackedEntities) {
		this->update(entry); // Determine what packets to send
		for (auto it = entry.m_visibleTo.begin(); it != entry.m_visibleTo.end();) {
			EntityId playerId = *it;
			auto playerIt = m_trackedEntities.find(playerId);
			if (playerIt == m_trackedEntities.end()) {
				it = entry.m_visibleTo.erase(it);
				continue;
			}
			auto& player = playerIt->second;

			auto distanceTo = std::abs(std::max(std::abs(entry.m_entity->m_position.m_x - player.m_entity->m_position.m_x),
			                                    std::abs(entry.m_entity->m_position.m_z - player.m_entity->m_position.m_z)));
			if (distanceTo > entry.m_profile.m_range) {
				auto& pSession = m_server->getSessionById(playerId);
				Packet::DespawnEntity pkt;
				pkt.m_entity_id = entry.m_entity->m_id;
				pkt.Serialize(pSession.m_stream);

				it = entry.m_visibleTo.erase(it);
				GlobalLogger().m_info << "Sent despawn packet to session id " << playerId << " for entity "
				                    << pkt.m_entity_id << "\n";
			} else {
				++it;
			}
		}
	}

	// Spawn pass
	for (EntityId playerId : m_playerIds) {
		auto& player = m_trackedEntities.at(playerId);

		for (auto& [entityId, entityEntry] : m_trackedEntities) {
			if (entityId == playerId)
				continue;

			auto distanceTo = std::abs(std::max(std::abs(entityEntry.m_entity->m_position.m_x - player.m_entity->m_position.m_x),
			                                    std::abs(entityEntry.m_entity->m_position.m_z - player.m_entity->m_position.m_z)));
			if (distanceTo > entityEntry.m_profile.m_range ||
			    entityEntry.m_visibleTo.find(playerId) != entityEntry.m_visibleTo.end()) {
				continue;
			}

			spawnEntityForPlayer(playerId, entityEntry);
		}
	}
}

void EntityTracker::spawnEntityForPlayer(EntityId playerId, TrackedEntry& entityEntry) {
	GlobalLogger().m_info << "Spawning entity at " << entityEntry.m_entity->m_position << "\n";
	auto& pSession = m_server->getSessionById(playerId);
	switch (entityEntry.m_entity->m_type) {
	case EntityType::ITEM: {
		GlobalLogger().m_info << "Spawned item entity\n";
		ItemEntity& ie = dynamic_cast<ItemEntity&>(*entityEntry.m_entity);
		Packet::SpawnItem pkt;
		pkt.m_entity_id = entityEntry.m_entity->m_id;
		pkt.m_item = ie.m_itemStack;
		pkt.m_q_position = quantizePosition(entityEntry.m_entity->m_position);
		// For some reason notch decided this should be a convoluted way of getting the initial spawn velocity
		auto quantizeSpawnVelocity = [](double v) -> int8_t {
			return int8_t(v * 128.0);
		};
		pkt.m_q_rotation = { quantizeSpawnVelocity(entityEntry.m_entity->m_velocity.m_x),
			               quantizeSpawnVelocity(entityEntry.m_entity->m_velocity.m_y),
			               quantizeSpawnVelocity(entityEntry.m_entity->m_velocity.m_z) };
		pkt.Serialize(pSession.m_stream);
		break;
	}
	case EntityType::PLAYER: {
		GlobalLogger().m_info << "Spawned player entity\n";
		Packet::SpawnPlayer pkt;
		pkt.m_entity_id = entityEntry.m_entity->m_id;
		pkt.m_held_item_id = Items::Id::NONE;
		pkt.m_q_position = quantizePosition(entityEntry.m_entity->m_position);
		pkt.m_q_rotation = { int8_t(quantizeRotation(entityEntry.m_entity->m_rotationYaw)),
			               int8_t(quantizeRotation(entityEntry.m_entity->m_rotationPitch)) };

		// To prevent bad behavior when we share a name with another entity
		auto username = m_server->getUsernameByEntityId(entityEntry.m_entity->m_id);
		pkt.m_username = username;
		pkt.Serialize(pSession.m_stream);
		break;
	}
	case EntityType::CREEPER: {
		Packet::SpawnMob pkt;
		pkt.m_entity_id = entityEntry.m_entity->m_id;
		pkt.m_mob_type = PacketData::MobType::CREEPER;
		pkt.m_q_position = quantizePosition(entityEntry.m_entity->m_position);
		pkt.m_q_rotation = { int8_t(quantizeRotation(entityEntry.m_entity->m_rotationYaw)),
			               int8_t(quantizeRotation(entityEntry.m_entity->m_rotationPitch)) };
		pkt.m_metadata.push_back(PacketData::EntityMetadata::DataEntry{ PacketData::EntityMetadata::BYTE, 0, int8_t(0) });
		pkt.Serialize(pSession.m_stream);
		break;
	}
	case EntityType::BOAT: {
		Packet::SpawnObject pkt;
		pkt.m_entity_id = entityEntry.m_entity->m_id;
		pkt.m_object_type = PacketData::ObjectType::BOAT;
		pkt.m_q_position = quantizePosition(entityEntry.m_entity->m_position);
		pkt.Serialize(pSession.m_stream);
		break;
	}
	case EntityType::FALLING_SAND: {
		GlobalLogger().m_info << "Spawned falling sand entity\n";
		Packet::SpawnObject pkt;
		pkt.m_entity_id = entityEntry.m_entity->m_id;
		pkt.m_object_type = PacketData::ObjectType::FALLING_SAND;
		pkt.m_q_position = quantizePosition(entityEntry.m_entity->m_position);
		pkt.m_q_velocity = quantizeVelocity(entityEntry.m_entity->m_velocity);
		pkt.Serialize(pSession.m_stream);
		break;
	}
	case EntityType::FALLING_GRAVEL: {
		GlobalLogger().m_info << "Spawned falling gravel entity\n";
		Packet::SpawnObject pkt;
		pkt.m_entity_id = entityEntry.m_entity->m_id;
		pkt.m_object_type = PacketData::ObjectType::FALLING_GRAVEL;
		pkt.m_q_position = quantizePosition(entityEntry.m_entity->m_position);
		pkt.m_q_velocity = quantizeVelocity(entityEntry.m_entity->m_velocity);
		pkt.Serialize(pSession.m_stream);
		break;
	}
	default:
		// TODO: Implement other types
		return;
	}
	entityEntry.m_visibleTo.insert(playerId);

	if (entityEntry.m_profile.m_sendVelocity) {
		// If velocity is enabled immediately send a follow up
		Packet::EntityVelocity velPkt;
		velPkt.m_entity_id = entityEntry.m_entity->m_id;
		velPkt.m_velocity = quantizeVelocity(entityEntry.m_entity->m_velocity);
		velPkt.Serialize(pSession.m_stream);
	}
}

void EntityTracker::despawnEntityForViewers(EntityId entityId, TrackedEntry& entry) {
	for (EntityId viewerId : entry.m_visibleTo) {
		auto& pSession = m_server->getSessionById(viewerId);
		Packet::DespawnEntity pkt;
		pkt.m_entity_id = entityId;
		pkt.Serialize(pSession.m_stream);
	}
}

void EntityTracker::sendPacketToPlayersInTrackedEntry(Packet::BasePacket& pkt, TrackedEntry& trackedEntry) {
	for (auto& playerId : trackedEntry.m_visibleTo) {
		pkt.Serialize(m_server->getSessionById(playerId).m_stream);
	}
}

TrackedEntry& EntityTracker::getTrackerForEntityId(EntityId id) {
	for (auto& [entityId, entityEntry] : m_trackedEntities) {
		if (entityId == id)
			return entityEntry;
	}
	throw std::out_of_range("Entity not found");
}

void EntityTracker::sendPacketToViewers(Packet::BasePacket& pkt, EntityId id) {
	for (auto& playerId : m_playerIds) {
		auto& playerEntry = getTrackerForEntityId(playerId);
		if (playerEntry.m_visibleTo.contains(id)) {
			pkt.Serialize(m_server->getSessionById(playerId).m_stream);
		}
	}
}

void EntityTracker::update(TrackedEntry& trackedEntry) {
	auto& entity = trackedEntry.m_entity;

	// Dirty flag gets checked every tick
	if (entity->m_forceVelocityUpdate) {
		entity->m_forceVelocityUpdate = false;
		trackedEntry.m_lastBroadcastMotion = { entity->m_velocity.m_x, entity->m_velocity.m_y, entity->m_velocity.m_z };
		Packet::EntityVelocity pkt;
		pkt.m_entity_id = entity->m_id;
		pkt.m_velocity = { quantizeVelocityComponent(entity->m_velocity.m_x), quantizeVelocityComponent(entity->m_velocity.m_y),
			             quantizeVelocityComponent(entity->m_velocity.m_z) };
		sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
	}

	trackedEntry.m_ticksSinceTeleport++;
	trackedEntry.m_updateCounter++;

	bool needsMovementUpdate = trackedEntry.m_updateCounter >= trackedEntry.m_profile.m_updateFrequency ||
	                           trackedEntry.m_ticksSinceTeleport >= m_forceTeleportTicks;

	if (needsMovementUpdate) {
		trackedEntry.m_updateCounter = 0;

		// The threshold-based velocity check
		if (trackedEntry.m_profile.m_sendVelocity) {
			Vec3 currentMotion = { entity->m_velocity.m_x, entity->m_velocity.m_y, entity->m_velocity.m_z };
			Vec3& lastMotion = trackedEntry.m_lastBroadcastMotion;
			double dmx = currentMotion.m_x - lastMotion.m_x;
			double dmy = currentMotion.m_y - lastMotion.m_y;
			double dmz = currentMotion.m_z - lastMotion.m_z;
			double deltaSq = dmx * dmx + dmy * dmy + dmz * dmz;
			const double motionThreshold = 0.02;

			bool needsVelocityUpdate = deltaSq > motionThreshold * motionThreshold ||
			                           (deltaSq > 0.0 && currentMotion.m_x == 0.0 && currentMotion.m_y == 0.0 &&
			                            currentMotion.m_z == 0.0);

			if (needsVelocityUpdate) {
				lastMotion = currentMotion;
				Packet::EntityVelocity pkt;
				pkt.m_entity_id = entity->m_id;
				pkt.m_velocity = { quantizeVelocityComponent(entity->m_velocity.m_x),
					             quantizeVelocityComponent(entity->m_velocity.m_y),
					             quantizeVelocityComponent(entity->m_velocity.m_z) };
				sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
			}
		}

		int32_t qx = quantizePositionComponent(entity->m_position.m_x);
		int32_t qy = quantizePositionComponent(entity->m_position.m_y);
		int32_t qz = quantizePositionComponent(entity->m_position.m_z);
		int32_t qYaw = quantizeRotation(entity->m_rotationYaw);
		int32_t qPitch = quantizeRotation(entity->m_rotationPitch);

		int32_t dx = qx - trackedEntry.m_lastEncodedPos.m_x;
		int32_t dy = qy - trackedEntry.m_lastEncodedPos.m_y;
		int32_t dz = qz - trackedEntry.m_lastEncodedPos.m_z;

		bool needsTP = dx < -128 || dx >= 128 || dy < -128 || dy >= 128 || dz < -128 || dz >= 128 ||
		               trackedEntry.m_ticksSinceTeleport >= m_forceTeleportTicks;

		if (needsTP) {
			trackedEntry.m_ticksSinceTeleport = 0;

			// resyncs the entity position
			entity->m_position.m_x = double(qx) / 32.0;
			entity->m_position.m_y = double(qy) / 32.0;
			entity->m_position.m_z = double(qz) / 32.0;
			entity->rebuildCollider();

			Packet::TeleportEntity pkt;
			pkt.m_entity_id = entity->m_id;
			pkt.m_position = { qx, qy, qz };
			pkt.m_rotation = { int8_t(qYaw), int8_t(qPitch) };
			sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
			trackedEntry.m_lastEncodedPos = { qx, qy, qz };
			trackedEntry.m_lastEncodedYaw = qYaw;
			trackedEntry.m_lastEncodedPitch = qPitch;
		} else {
			bool needsRelMove = dx != 0 || dy != 0 || dz != 0;
			bool needsRot = qYaw != trackedEntry.m_lastEncodedYaw || qPitch != trackedEntry.m_lastEncodedPitch;

			if (needsRelMove && needsRot) {
				Packet::EntityPositionAndRotation pkt;
				pkt.m_qr_position = { int8_t(dx), int8_t(dy), int8_t(dz) };
				pkt.m_q_rotation = { int8_t(qYaw), int8_t(qPitch) };
				pkt.m_entity_id = entity->m_id;
				sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
				trackedEntry.m_lastEncodedPos = { qx, qy, qz };
				trackedEntry.m_lastEncodedYaw = qYaw;
				trackedEntry.m_lastEncodedPitch = qPitch;
				return;
			};
			if (needsRelMove) {
				Packet::EntityPosition pkt;
				pkt.m_qr_position = { int8_t(dx), int8_t(dy), int8_t(dz) };
				pkt.m_entity_id = entity->m_id;
				sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
				trackedEntry.m_lastEncodedPos = { qx, qy, qz };
				return;
			}
			if (needsRot) {
				Packet::EntityRotation pkt;
				pkt.m_q_rotation = { int8_t(qYaw), int8_t(qPitch) };
				pkt.m_entity_id = entity->m_id;
				sendPacketToPlayersInTrackedEntry(pkt, trackedEntry);
				trackedEntry.m_lastEncodedYaw = qYaw;
				trackedEntry.m_lastEncodedPitch = qPitch;
				return;
			}
		}
	}
}