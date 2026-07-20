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
#include "logger.h"
#include "packet_data.h"
#include <algorithm>
#include <cstdint>

// Update each player instance so entities properly despawn and spawn for them
void EntityTracker::Tick() {
	std::vector<EntityId> deadThisTick;

	for (auto& [entityId, entry] : trackedEntities) {
		if (entry.entity->isDead)
			deadThisTick.push_back(entry.entity->id);
	}

	for (auto& entityId : deadThisTick) {
		auto& entry = trackedEntities.at(entityId);
		DespawnEntityForViewers(entityId, entry);
		for (auto& [id, otherEntry] : trackedEntities) {
			otherEntry.visibleTo.erase(entityId);
		}
		trackedEntities.erase(entityId);
		playerIds.erase(entityId);
	}

	// Despawn pass / update
	for (auto& [entityId, entry] : trackedEntities) {
		this->Update(entry); // Determine what packets to send
		for (auto it = entry.visibleTo.begin(); it != entry.visibleTo.end();) {
			EntityId playerId = *it;
			auto playerIt = trackedEntities.find(playerId);
			if (playerIt == trackedEntities.end()) {
				it = entry.visibleTo.erase(it);
				continue;
			}
			auto& player = playerIt->second;

			auto distanceTo = std::abs(std::max(std::abs(entry.entity->position.x - player.entity->position.x),
			                                    std::abs(entry.entity->position.z - player.entity->position.z)));
			if (distanceTo > entry.profile.range) {
				auto pSession = server->GetSessionById(playerId);
				if (!pSession)
					continue;
				Packet::DespawnEntity pkt;
				pkt.entityId = entry.entity->id;
				pkt.Serialize(pSession->stream);

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

			auto distanceTo = std::abs(std::max(std::abs(entityEntry.entity->position.x - player.entity->position.x),
			                                    std::abs(entityEntry.entity->position.z - player.entity->position.z)));
			if (distanceTo > entityEntry.profile.range ||
			    entityEntry.visibleTo.find(playerId) != entityEntry.visibleTo.end()) {
				continue;
			}

			SpawnEntityForPlayer(playerId, entityEntry);
		}
	}
}

void EntityTracker::SpawnEntityForPlayer(EntityId _playerId, TrackedEntry& _entityEntry) {
	auto pSession = server->GetSessionById(_playerId);
	if (!pSession)
		return;
	switch (_entityEntry.entity->type) {
	case EntityType::ITEM: {
		ItemEntity& ie = dynamic_cast<ItemEntity&>(*_entityEntry.entity);
		Packet::SpawnItem pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.item = ie.itemStack;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		// For some reason notch decided this should be a convoluted way of getting the initial spawn velocity
		auto quantizeSpawnVelocity = [](double _v) -> int8_t {
			return int8_t(_v * 128.0);
		};
		pkt.qRotation = { quantizeSpawnVelocity(_entityEntry.entity->velocity.x),
			              quantizeSpawnVelocity(_entityEntry.entity->velocity.y),
			              quantizeSpawnVelocity(_entityEntry.entity->velocity.z) };
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::PLAYER: {
		Packet::SpawnPlayer pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.heldItemId = Items::Id::NONE;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qRotation = { int8_t(QuantizeRotation(_entityEntry.entity->rotationYaw)),
			              int8_t(QuantizeRotation(_entityEntry.entity->rotationPitch)) };

		// To prevent bad behavior when we share a name with another entity
		auto username = server->GetUsernameByEntityId(_entityEntry.entity->id);
		if (username.empty()) {
			GlobalLogger().warn << "Refused to spawn player entity, as no username was found!\n";
			return;
		}
		pkt.username = username;
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::CREEPER: {
		Packet::SpawnMob pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.mobType = PacketData::MobType::CREEPER;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qRotation = { int8_t(QuantizeRotation(_entityEntry.entity->rotationYaw)),
			              int8_t(QuantizeRotation(_entityEntry.entity->rotationPitch)) };
		pkt.metadata.push_back(PacketData::EntityMetadata::DataEntry{ PacketData::EntityMetadata::BYTE, 0, int8_t(0) });
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::BOAT: {
		Packet::SpawnObject pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.objectType = PacketData::ObjectType::BOAT;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::FALLING_SAND: {
		Packet::SpawnObject pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.objectType = PacketData::ObjectType::FALLING_SAND;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qVelocity = QuantizeVelocity(_entityEntry.entity->velocity);
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::FALLING_GRAVEL: {
		Packet::SpawnObject pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.objectType = PacketData::ObjectType::FALLING_GRAVEL;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qVelocity = QuantizeVelocity(_entityEntry.entity->velocity);
		pkt.Serialize(pSession->stream);
		break;
	}
	default:
		// TODO: Implement other types
		return;
	}
	_entityEntry.visibleTo.insert(_playerId);

	if (_entityEntry.profile.sendVelocity) {
		// If velocity is enabled immediately send a follow up
		Packet::EntityVelocity velPkt;
		velPkt.entityId = _entityEntry.entity->id;
		velPkt.velocity = QuantizeVelocity(_entityEntry.entity->velocity);
		velPkt.Serialize(pSession->stream);
	}
}

void EntityTracker::DespawnEntityForViewers(EntityId _entityId, TrackedEntry& _entry) {
	for (EntityId viewerId : _entry.visibleTo) {
		auto pSession = server->GetSessionById(viewerId);
		if (!pSession)
			continue;
		Packet::DespawnEntity pkt;
		pkt.entityId = _entityId;
		pkt.Serialize(pSession->stream);
	}
}

void EntityTracker::SendPacketToPlayersInTrackedEntry(Packet::BasePacket& _pkt, TrackedEntry& _trackedEntry) {
	for (auto& playerId : _trackedEntry.visibleTo) {
		auto session = server->GetSessionById(playerId);
		if (!session)
			continue;
		_pkt.Serialize(session->stream);
	}
}

TrackedEntry& EntityTracker::GetTrackerForEntityId(EntityId _id) {
	for (auto& [entityId, entityEntry] : trackedEntities) {
		if (entityId == _id)
			return entityEntry;
	}
	throw std::out_of_range("Entity not found");
}

void EntityTracker::SendPacketToViewers(Packet::BasePacket& _pkt, EntityId _id) {
	for (auto& playerId : playerIds) {
		auto& playerEntry = GetTrackerForEntityId(playerId);
		if (playerEntry.visibleTo.contains(_id)) {
			auto session = server->GetSessionById(playerId);
			if (!session)
				continue;
			_pkt.Serialize(session->stream);
		}
	}
}

void EntityTracker::Update(TrackedEntry& _trackedEntry) {
	auto& entity = _trackedEntry.entity;

	// Dirty flag gets checked every Tick
	if (entity->forceVelocityUpdate) {
		entity->forceVelocityUpdate = false;
		_trackedEntry.lastBroadcastMotion = { entity->velocity.x, entity->velocity.y, entity->velocity.z };
		Packet::EntityVelocity pkt;
		pkt.entityId = entity->id;
		pkt.velocity = { QuantizeVelocityComponent(entity->velocity.x), QuantizeVelocityComponent(entity->velocity.y),
			             QuantizeVelocityComponent(entity->velocity.z) };
		SendPacketToPlayersInTrackedEntry(pkt, _trackedEntry);

		// If we are a player then we need to recieve this velocity update
		if (auto thisSession = server->GetSessionById(entity->id)) {
			pkt.Serialize(thisSession->stream);
		}
	}

	_trackedEntry.ticksSinceTeleport++;
	_trackedEntry.updateCounter++;

	bool needsMovementUpdate = _trackedEntry.updateCounter >= _trackedEntry.profile.updateFrequency ||
	                           _trackedEntry.ticksSinceTeleport >= forceTeleportTicks;

	if (needsMovementUpdate) {
		_trackedEntry.updateCounter = 0;

		// The threshold-based velocity check
		if (_trackedEntry.profile.sendVelocity) {
			Vec3 currentMotion = { entity->velocity.x, entity->velocity.y, entity->velocity.z };
			Vec3& lastMotion = _trackedEntry.lastBroadcastMotion;
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
				pkt.entityId = entity->id;
				pkt.velocity = { QuantizeVelocityComponent(entity->velocity.x),
					             QuantizeVelocityComponent(entity->velocity.y),
					             QuantizeVelocityComponent(entity->velocity.z) };
				SendPacketToPlayersInTrackedEntry(pkt, _trackedEntry);
			}
		}

		int32_t qx = QuantizePositionComponent(entity->position.x);
		int32_t qy = QuantizePositionComponent(entity->position.y);
		int32_t qz = QuantizePositionComponent(entity->position.z);
		int32_t qYaw = QuantizeRotation(entity->rotationYaw);
		int32_t qPitch = QuantizeRotation(entity->rotationPitch);

		int32_t dx = qx - _trackedEntry.lastEncodedPos.x;
		int32_t dy = qy - _trackedEntry.lastEncodedPos.y;
		int32_t dz = qz - _trackedEntry.lastEncodedPos.z;

		bool needsTP = dx < -128 || dx >= 128 || dy < -128 || dy >= 128 || dz < -128 || dz >= 128 ||
		               _trackedEntry.ticksSinceTeleport >= forceTeleportTicks;

		if (needsTP) {
			_trackedEntry.ticksSinceTeleport = 0;

			// resyncs the entity position
			entity->position.x = double(qx) / 32.0;
			entity->position.y = double(qy) / 32.0;
			entity->position.z = double(qz) / 32.0;
			entity->RebuildCollider();

			Packet::TeleportEntity pkt;
			pkt.entityId = entity->id;
			pkt.position = { qx, qy, qz };
			pkt.rotation = { int8_t(qYaw), int8_t(qPitch) };
			SendPacketToPlayersInTrackedEntry(pkt, _trackedEntry);
			_trackedEntry.lastEncodedPos = { qx, qy, qz };
			_trackedEntry.lastEncodedYaw = qYaw;
			_trackedEntry.lastEncodedPitch = qPitch;
		} else {
			bool needsRelMove = dx != 0 || dy != 0 || dz != 0;
			bool needsRot = qYaw != _trackedEntry.lastEncodedYaw || qPitch != _trackedEntry.lastEncodedPitch;

			if (needsRelMove && needsRot) {
				Packet::EntityPositionAndRotation pkt;
				pkt.qrPosition = { int8_t(dx), int8_t(dy), int8_t(dz) };
				pkt.qRotation = { int8_t(qYaw), int8_t(qPitch) };
				pkt.entityId = entity->id;
				SendPacketToPlayersInTrackedEntry(pkt, _trackedEntry);
				_trackedEntry.lastEncodedPos = { qx, qy, qz };
				_trackedEntry.lastEncodedYaw = qYaw;
				_trackedEntry.lastEncodedPitch = qPitch;
				return;
			};
			if (needsRelMove) {
				Packet::EntityPosition pkt;
				pkt.qrPosition = { int8_t(dx), int8_t(dy), int8_t(dz) };
				pkt.entityId = entity->id;
				SendPacketToPlayersInTrackedEntry(pkt, _trackedEntry);
				_trackedEntry.lastEncodedPos = { qx, qy, qz };
				return;
			}
			if (needsRot) {
				Packet::EntityRotation pkt;
				pkt.qRotation = { int8_t(qYaw), int8_t(qPitch) };
				pkt.entityId = entity->id;
				SendPacketToPlayersInTrackedEntry(pkt, _trackedEntry);
				_trackedEntry.lastEncodedYaw = qYaw;
				_trackedEntry.lastEncodedPitch = qPitch;
				return;
			}
		}
	}
}