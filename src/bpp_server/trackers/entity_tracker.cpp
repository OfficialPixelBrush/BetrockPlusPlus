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
#include "entities/entity_mobile.h"
#include "logger.h"
#include "packet_data.h"
#include <algorithm>
#include <cstdint>

double DistanceBetweenPlayerAndEntity(Entity* _entity, Entity* _player) {
	auto dx = _entity->position.x - _player->position.x;
	auto dz = _entity->position.z - _player->position.z;
	return dx * dx + dz * dz;
}

// Update each player instance so entities properly despawn and spawn for them
void EntityTracker::Tick() {
	std::vector<EntityId> deadThisTick;

	for (auto& [entityId, entry] : trackedEntities) {
		if (entry.entity->isDead) {
			deadThisTick.push_back(entry.entity->id);
		}
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

			if (DistanceBetweenPlayerAndEntity(entry.entity, player.entity) > entry.profile.range*entry.profile.range) {
				auto pSession = server->GetSessionById(playerId);
				if (!pSession) {
					it = entry.visibleTo.erase(it);
					continue;
				}
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

			if (DistanceBetweenPlayerAndEntity(entityEntry.entity, player.entity) > entityEntry.profile.range*entityEntry.profile.range ||
			    entityEntry.visibleTo.find(playerId) != entityEntry.visibleTo.end()) {
				continue;
			}

			SpawnEntityForPlayer(playerId, entityEntry);
		}
	}
}

void EntityTracker::TrackEntity(Entity* _entity) {
	TrackedEntry entry;
	entry.entity = _entity;
	entry.profile = GetTrackingProfile(*_entity);

	entry.lastEncodedPos = QuantizePosition(_entity->position);
	entry.lastBroadcastMotion = _entity->velocity;
	entry.lastEncodedPitch = QuantizeRotation(_entity->rotationPitch);
	entry.lastEncodedYaw = QuantizeRotation(_entity->rotationYaw);
	entry.updateCounter = entry.profile.updateFrequency > 0 ? rand.NextInt(entry.profile.updateFrequency) : 0;

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
		if (DistanceBetweenPlayerAndEntity(entry.entity, player.entity) > newEntry.profile.range*newEntry.profile.range)
			continue;
		// Register the viewer before spawning
		newEntry.visibleTo.insert(playerId);
		SpawnEntityForPlayer(playerId, newEntry);
	}

	// Force an update
	Update(newEntry);
}

void EntityTracker::UntrackEntity(Entity* _entity) {
	auto it = trackedEntities.find(_entity->id);
	if (it == trackedEntities.end())
		return;

	DespawnEntityForViewers(_entity->id, it->second);

	for (auto& [id, otherEntry] : trackedEntities)
		otherEntry.visibleTo.erase(_entity->id);

	trackedEntities.erase(it);
	playerIds.erase(_entity->id);
}

void EntityTracker::AddPlayer(Entity* _player) {
	TrackedEntry entry;
	entry.entity = _player;
	entry.profile = GetTrackingProfile(*_player);

	entry.lastEncodedPos = QuantizePosition(_player->position);
	entry.lastBroadcastMotion = _player->velocity;
	entry.lastEncodedPitch = QuantizeRotation(_player->rotationPitch);
	entry.lastEncodedYaw = QuantizeRotation(_player->rotationYaw);
	entry.updateCounter = entry.profile.updateFrequency > 0 ? rand.NextInt(entry.profile.updateFrequency) : 0;

	trackedEntities[_player->id] = std::move(entry);
	playerIds.insert(_player->id);
	auto& newPlayerEntry = trackedEntities.at(_player->id);

	// This new player should immediately see anything already in range
	for (auto& [entityId, entityEntry] : trackedEntities) {
		if (entityId == _player->id)
			continue;
		if (DistanceBetweenPlayerAndEntity(entry.entity, newPlayerEntry.entity) > entityEntry.profile.range*entityEntry.profile.range)
			continue;
		// Register the viewer before spawning
		entityEntry.visibleTo.insert(_player->id);
		SpawnEntityForPlayer(_player->id, entityEntry);
	}

	for (EntityId otherPlayerId : playerIds) {
		if (otherPlayerId == _player->id)
			continue;
		auto otherIt = trackedEntities.find(otherPlayerId);
		if (otherIt == trackedEntities.end())
			continue;
		if (DistanceBetweenPlayerAndEntity(entry.entity, newPlayerEntry.entity) > newPlayerEntry.profile.range*newPlayerEntry.profile.range)
			continue;
		newPlayerEntry.visibleTo.insert(otherPlayerId);
		SpawnEntityForPlayer(otherPlayerId, newPlayerEntry);
	}

	// Force an update
	Update(newPlayerEntry);
}

void EntityTracker::RemovePlayer(Entity* _player) {
	// Despawn all entities that we are viewing
	for (auto& [id, otherEntry] : trackedEntities)
		if (otherEntry.visibleTo.contains(_player->id)) {
			// Send a despawn packet to the client
			Packet::DespawnEntity pkt;
			pkt.entityId = otherEntry.entity->id;
			auto playerSession = server->GetSessionById(_player->id);
			if (playerSession)
				pkt.Serialize(playerSession->stream);
		}

	// Fully untrack this entity as usual
	UntrackEntity(_player);
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
			// Fallback
			auto& playerEntity = dynamic_cast<EntityMPPlayer&>(*_entityEntry.entity);
			username = playerEntity.session ? playerEntity.session->username : " ";
		}
		pkt.username = username;
		pkt.Serialize(pSession->stream);
		SendEquipmentState(_entityEntry, pSession);
		SendMetadataState(_entityEntry, pSession);
		break;
	}
	case EntityType::CREEPER: {
		Packet::SpawnMob pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.mobType = PacketData::MobType::CREEPER;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qRotation = { int8_t(QuantizeRotation(_entityEntry.entity->rotationYaw)),
			              int8_t(QuantizeRotation(_entityEntry.entity->rotationPitch)) };
		_entityEntry.entity->EncodeMetadata(pkt.metadata);
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::SPIDER: {
		Packet::SpawnMob pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.mobType = PacketData::MobType::SPIDER;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qRotation = { int8_t(QuantizeRotation(_entityEntry.entity->rotationYaw)),
			              int8_t(QuantizeRotation(_entityEntry.entity->rotationPitch)) };
		_entityEntry.entity->EncodeMetadata(pkt.metadata);
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::ZOMBIE: {
		Packet::SpawnMob pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.mobType = PacketData::MobType::ZOMBIE;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qRotation = { int8_t(QuantizeRotation(_entityEntry.entity->rotationYaw)),
			              int8_t(QuantizeRotation(_entityEntry.entity->rotationPitch)) };
		_entityEntry.entity->EncodeMetadata(pkt.metadata);
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::SKELETON: {
		Packet::SpawnMob pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.mobType = PacketData::MobType::SKELETON;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qRotation = { int8_t(QuantizeRotation(_entityEntry.entity->rotationYaw)),
			              int8_t(QuantizeRotation(_entityEntry.entity->rotationPitch)) };
		_entityEntry.entity->EncodeMetadata(pkt.metadata);
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::PIG: {
		Packet::SpawnMob pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.mobType = PacketData::MobType::PIG;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qRotation = { int8_t(QuantizeRotation(_entityEntry.entity->rotationYaw)),
			              int8_t(QuantizeRotation(_entityEntry.entity->rotationPitch)) };
		_entityEntry.entity->EncodeMetadata(pkt.metadata);
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::COW: {
		Packet::SpawnMob pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.mobType = PacketData::MobType::COW;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qRotation = { int8_t(QuantizeRotation(_entityEntry.entity->rotationYaw)),
			              int8_t(QuantizeRotation(_entityEntry.entity->rotationPitch)) };
		_entityEntry.entity->EncodeMetadata(pkt.metadata);
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::SHEEP: {
		Packet::SpawnMob pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.mobType = PacketData::MobType::SHEEP;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qRotation = { int8_t(QuantizeRotation(_entityEntry.entity->rotationYaw)),
			              int8_t(QuantizeRotation(_entityEntry.entity->rotationPitch)) };
		_entityEntry.entity->EncodeMetadata(pkt.metadata);
		pkt.Serialize(pSession->stream);
		break;
	}
	case EntityType::CHICKEN: {
		Packet::SpawnMob pkt;
		pkt.entityId = _entityEntry.entity->id;
		pkt.mobType = PacketData::MobType::CHICKEN;
		pkt.qPosition = QuantizePosition(_entityEntry.entity->position);
		pkt.qRotation = { int8_t(QuantizeRotation(_entityEntry.entity->rotationYaw)),
			              int8_t(QuantizeRotation(_entityEntry.entity->rotationPitch)) };
		_entityEntry.entity->EncodeMetadata(pkt.metadata);
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
		GlobalLogger().warn << "Unhandled entity type: " << int(_entityEntry.entity->type) << "\n";
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

TrackedEntry* EntityTracker::GetTrackerForEntityId(EntityId _id) {
	auto it = trackedEntities.find(_id);
	return it != trackedEntities.end() ? &it->second : nullptr;
}

void EntityTracker::SendPacketToViewers(Packet::BasePacket& _pkt, EntityId _id) {
	auto* entry = GetTrackerForEntityId(_id);
	if (!entry)
		return; // entity isn't tracked
	for (EntityId viewerId : entry->visibleTo) {
		auto session = server->GetSessionById(viewerId);
		if (!session)
			continue;
		_pkt.Serialize(session->stream);
	}
}

void EntityTracker::SendMetadataState(TrackedEntry& _trackedEntry, std::shared_ptr<PlayerSession> _targetSession) {
	Packet::EntityMetadata pkt;
	pkt.entityId = _trackedEntry.entity->id;
	_trackedEntry.entity->EncodeMetadata(pkt.metadata);
	pkt.Serialize(_targetSession->stream);
}

void EntityTracker::UpdateMetadataState(TrackedEntry& _trackedEntry) {
	if (!_trackedEntry.entity->wasMetadataUpdated)
		return;
	Packet::EntityMetadata pkt;
	pkt.entityId = _trackedEntry.entity->id;
	_trackedEntry.entity->EncodeMetadata(pkt.metadata);
	SendPacketToViewers(pkt, _trackedEntry.entity->id);
	_trackedEntry.entity->wasMetadataUpdated = false;

	// Send the packet to ourselves if we are a player
	auto mpe = dynamic_cast<EntityMPPlayer*>(_trackedEntry.entity);
	if (mpe && mpe->session) {
		pkt.Serialize(mpe->session->stream);
	}
}

// Used when we first spawn an entity for a specific player
void EntityTracker::SendEquipmentState(TrackedEntry& _trackedEntry, std::shared_ptr<PlayerSession> _targetSession) {
	auto updateEquipmentSlot = [&](int _slot, ItemStack* _stack) -> void {
		if (!_stack || !_targetSession)
			return;
		Packet::SetEquipment pkt;
		pkt.entityId = _trackedEntry.entity->id;
		pkt.inventorySlot = _slot;
		pkt.itemId = _stack->id;
		pkt.itemMetadata = _stack->data;

		pkt.Serialize(_targetSession->stream);
	};

	MobileEntity& entity = dynamic_cast<MobileEntity&>(*_trackedEntry.entity);
	updateEquipmentSlot(4, entity.armor[0]);
	updateEquipmentSlot(3, entity.armor[1]);
	updateEquipmentSlot(2, entity.armor[2]);
	updateEquipmentSlot(1, entity.armor[3]);
	updateEquipmentSlot(0, &entity.heldItem);
}

// Update our equipment
void EntityTracker::UpdateEquipmentState(TrackedEntry& _trackedEntry) {
	auto updateEquipmentSlot = [&](int _slot, ItemStack* _stack) -> void {
		if (!_stack)
			return;
		Packet::SetEquipment pkt;
		pkt.entityId = _trackedEntry.entity->id;
		pkt.inventorySlot = _slot;
		pkt.itemId = _stack->id;
		pkt.itemMetadata = _stack->data;

		SendPacketToViewers(pkt, _trackedEntry.entity->id);
	};

	auto& equipment = _trackedEntry.equipmentProfile;
	MobileEntity& entity = dynamic_cast<MobileEntity&>(*_trackedEntry.entity);
	ItemStack none = {};
	auto helmet = entity.armor[0] ? entity.armor[0] : &none;
	auto chest = entity.armor[1] ? entity.armor[1] : &none;
	auto leg = entity.armor[2] ? entity.armor[2] : &none;
	auto boot = entity.armor[3] ? entity.armor[3] : &none;
	if (equipment.helmet != helmet->id) {
		updateEquipmentSlot(4, helmet);
		equipment.helmet = helmet->id;
	}
	if (equipment.chestplate != chest->id) {
		updateEquipmentSlot(3, chest);
		equipment.chestplate = chest->id;
	}
	if (equipment.legging != leg->id) {
		updateEquipmentSlot(2, leg);
		equipment.legging = leg->id;
	}
	if (equipment.boot != boot->id) {
		updateEquipmentSlot(1, boot);
		equipment.boot = boot->id;
	}
	if (equipment.heldItem != entity.heldItem) {
		updateEquipmentSlot(0, &entity.heldItem);
		equipment.heldItem = entity.heldItem;
	}
}

void EntityTracker::UpdateDamageState(TrackedEntry& _trackedEntry) {
	MobileEntity& entity = dynamic_cast<MobileEntity&>(*_trackedEntry.entity);

	// Send the death animation packet when the entity starts its death timer
	if (entity.deathTime == 1) {
		Packet::EntityEvent deathPkt;
		deathPkt.entityId = entity.id;
		deathPkt.action = PacketData::EntityEvent::DEATH;

		SendPacketToPlayersInTrackedEntry(deathPkt, _trackedEntry);
		return;
	}

	// Send the hurt animation packet / death animation packet whenever that happens.
	if (entity.lastHealth != entity.GetHeartsHealth() || entity.beenAttacked) {
		if (entity.GetHeartsHealth() - entity.lastHealth < 0 || entity.beenAttacked) {
			Packet::EntityEvent pkt;
			pkt.entityId = entity.id;
			pkt.action = PacketData::EntityEvent::HURT;

			SendPacketToPlayersInTrackedEntry(pkt, _trackedEntry);

			// If we are a player play the damage sound for ourselves too
			if (entity.type == EntityType::PLAYER) {
				auto session = server->GetSessionById(entity.id);
				if (session) {
					pkt.Serialize(session->stream);
				}
			}

			entity.beenAttacked = false;
		}

		entity.lastHealth = entity.GetHeartsHealth();
	}
}

void EntityTracker::Update(TrackedEntry& _trackedEntry) {
	auto& entity = _trackedEntry.entity;

	// If we are a mobile entity then send the damage state and update our equipment
	// TODO: Only send these if any of them have been updated
	if (std::find(this->mobileEntities.begin(), this->mobileEntities.end(), entity->type) !=
	    this->mobileEntities.end()) {
		UpdateDamageState(_trackedEntry);
		UpdateEquipmentState(_trackedEntry);
		UpdateMetadataState(_trackedEntry);
	}

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

			Packet::TeleportEntity pkt;
			pkt.entityId = entity->id;
			pkt.position = { qx, qy, qz };
			pkt.rotation = { int8_t(qYaw), int8_t(qPitch) };
			SendPacketToPlayersInTrackedEntry(pkt, _trackedEntry);
			_trackedEntry.lastEncodedPos = { qx, qy, qz };
			_trackedEntry.lastEncodedYaw = qYaw;
			_trackedEntry.lastEncodedPitch = qPitch;
		} else {
			bool needsRelMove =
				std::abs(dx) > MINIMUM_POSITION_DELTA ||
				std::abs(dy) > MINIMUM_POSITION_DELTA ||
				std::abs(dz) > MINIMUM_POSITION_DELTA;
			bool needsRot =
				std::abs(qYaw   - _trackedEntry.lastEncodedYaw)   > MINIMUM_ROTATION_DELTA ||
				std::abs(qPitch - _trackedEntry.lastEncodedPitch) > MINIMUM_ROTATION_DELTA;

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