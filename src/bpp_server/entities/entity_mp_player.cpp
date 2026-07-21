/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "entity_mp_player.h"
#include "../player_conn/player_session.h"
#include "entities/entity_item.h"
#include "entities/entity_player.h"
#include "inventory/item_stack.h"
#include "networking/network_stream.h"
#include "networking/packets.h"

bool EntityMPPlayer::PickupItem(ItemStack& _stack, EntityId _entityId) {
	if (this->session->inventory.PickupItem(_stack)) {
		Packet::CollectItem pkt;
		pkt.collectorEntityId = this->id;
		pkt.itemEntityId = _entityId;
		this->session->entityTracker->SendPacketToViewers(pkt, this->id);
		pkt.Serialize(this->session->stream);
		return true;
	}

	return false;
}

// This works over a copy of your item, it doesn't remove or decrement it !!!
bool EntityMPPlayer::DropItem(ItemStack _stack) {
	if (_stack.id == Items::Id::INVALID || _stack.count <= 0)
		return false;

	// Create the item entity
	Vec3 itemPos = { position.x, position.y - 0.3 + PLAYER_EYE_HEIGHT, position.z };
	std::shared_ptr<ItemEntity> itemEntity = std::make_shared<ItemEntity>(itemPos);
	itemEntity->itemStack = _stack;
	itemEntity->pickupCooldown = 40; // So we don't pick it up instantly

	// Give ourselves some random velocity based on look direction
	float _velocity = 0.3f;
	itemEntity->velocity.x = double(-std::sin(this->rotationYaw / 180.0F * JavaMath::PI_FLOAT) *
	                                std::cos(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * _velocity);
	itemEntity->velocity.z = double(std::cos(this->rotationYaw / 180.0F * JavaMath::PI_FLOAT) *
	                                std::cos(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * _velocity);
	itemEntity->velocity.y = double(-std::sin(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * _velocity + 0.1F);

	// Add a little bit of randomness
	_velocity = 0.02f;
	float angle = rand.NextFloat() * JavaMath::PI_FLOAT * 2.0f;
	_velocity *= rand.NextFloat();
	itemEntity->velocity.x += std::cos(angle) * _velocity;
	itemEntity->velocity.y += (rand.NextFloat() - rand.NextFloat()) * 0.1f;
	itemEntity->velocity.z += std::sin(angle) * _velocity;

	// Register our item with the world
	this->world->entityManager.AddEntity(std::move(itemEntity));
	return true;
}

void EntityMPPlayer::HandlePositionChecks() {
	if (session->pendingTeleport) {
		// We have a pending teleport. Check to see if the player caught up
		if (!session->pendingPosition)
			return;
		auto& pending = *session->pendingPosition;
		Vec3 claimed = { pending.x, pending.y + PLAYER_EYE_HEIGHT, pending.z };
		Vec3 delta = claimed - *session->pendingTeleport;
		auto dist = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
		if (dist > 0.0625) {
			// Player isn't at the teleported position so send another tp packet
			// Also reset our position
			auto& ptp = *session->pendingTeleport;
			this->Teleport({ ptp.x, ptp.y, ptp.z }, { rotationYaw, rotationPitch });
			session->position.pos = *session->pendingTeleport;
			Packet::PlayerPosition pkt;
			pkt.onGround = onGround;
			pkt.position = { position.x, position.y, position.z };
			pkt.cameraY = position.y; // This is backwards, thanks notch
			pkt.Serialize(session->stream);
			return;
		}
		// Client acknowledged our tp
		session->pendingTeleport.reset();
	}

	// Always trust rotations
	this->rotationYaw = session->rotation.x;
	this->rotationPitch = session->rotation.y;

	// If we recieved a movement packet this Tick do our server side checks
	if (session->pendingPosition) {
		// Re-simulate our move
		bool savedOnGround = onGround;
		bool residualTooLarge = false;
		bool movedWrong = false;
		bool wasClearBefore = world->GetCollidingBoundingBoxes(collider.Expand(-0.0625, -0.0625, -0.0625)).empty();
		Vec3 lastPosition = this->position;
		Vec3 claimed = *session->pendingPosition;
		Vec3 delta = claimed - lastPosition;
		if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z > 100.0) {
			GlobalLogger().warn << "Client " << session->username << " moved wrongly!\n";
			movedWrong = true;
		}
		Move(delta);
		movedThisTick = true;

		// Reset on ground to what the client last claimed
		onGround = savedOnGround;

		// Deal fall damage
		if (onGround) {
			if (fallDistance > FALL_DAMAGE_FLOOR) {
				AttackEntityFrom(nullptr, (int)std::ceil(fallDistance - FALL_DAMAGE_FLOOR));
			}
			fallDistance = 0;
		} else if (delta.y < 0) {
			fallDistance -= delta.y;
		}

		auto resolvedDelta = delta;

		// How far is our simulated move vs what the client says?
		delta = claimed - this->position;
		double residual = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
		if (residual < 0.0625) {
			this->position = claimed; // Trust it
			session->position.pos = claimed;
			this->velocity = resolvedDelta;
			RebuildCollider();
		} else {
			// Send a correction
			residualTooLarge = true;
		}

		bool clearNow = world->GetCollidingBoundingBoxes(collider.Expand(-0.0625, -0.0625, -0.0625)).empty();

		if ((wasClearBefore && (residualTooLarge || !clearNow)) || movedWrong) {
			// TP our player back
			GlobalLogger().info << "Residual from move was: " << residual << "\n";
			GlobalLogger().info << "Rubberbanded player! wasClear=" << wasClearBefore
			                    << " residual=" << residualTooLarge << " clearNow=" << clearNow
			                    << " movedWrong=" << movedWrong << " pos=(" << this->position.x << ","
			                    << this->position.y << "," << this->position.z << ")"
			                    << " colliderY=[" << collider.minY << "," << collider.maxY << "]"
			                    << "\n";
			this->Teleport(lastPosition, { rotationYaw, rotationPitch });
			session->position.pos = lastPosition;
			Packet::PlayerPosition pkt;
			pkt.onGround = onGround;
			pkt.position = { position.x, position.y + PLAYER_EYE_HEIGHT + 0.0625, position.z };
			pkt.cameraY = position.y; // This is backwards, thanks notch
			pkt.Serialize(session->stream);
		}

		session->pendingPosition.reset();
	}
}

void EntityMPPlayer::UpdateFallState(float _movedY) {
	return; // no-op
}

void EntityMPPlayer::DropInventory() {
	for (size_t i = 0; i < session->inventory.slots.size(); i++) {
		auto stack = session->inventory.GetStackInSlot(i);
		if (stack != nullptr) {
			// Create the item entity
			Vec3 itemPos = { position.x, position.y - 0.3 + PLAYER_EYE_HEIGHT, position.z };
			std::shared_ptr<ItemEntity> itemEntity = std::make_shared<ItemEntity>(itemPos);

			ItemStack newStack{ .id = stack->id, .count = stack->count, .data = stack->data };
			itemEntity->itemStack = std::move(newStack);
			itemEntity->pickupCooldown = 40; // So we don't pick it up instantly

			// Give ourselves some random velocity
			float _velocity = 0.3f;
			float _angle = this->rand.NextFloat() * float(JavaMath::PI) * 2.0f;
			itemEntity->velocity.x = double(-std::sin(_angle) * _velocity);
			itemEntity->velocity.z = double(std::cos(_angle) * _velocity);
			itemEntity->velocity.y = 0.2;

			// Register our item with the world
			this->world->entityManager.AddEntity(std::move(itemEntity));

			// Erase this item from the inventory
			stack->count = 0;
			stack->data = 0;
			stack->id = Items::Id::INVALID;
		}
	}
}

void EntityMPPlayer::OnDeath() {
	MobileEntity::OnDeath();
	this->DropInventory();
}

void EntityMPPlayer::Tick() {
	if (!session)
		return;

	if (!movedThisTick) {
		Vec3 none = { 0.0, 0.0, 0.0 };
		Move(none);
	}

	// Do living entity stuff
	MobileEntity::Tick();

	// Our health changed
	if (this->lastHealth != getHeartsHealth()) {
		Packet::SetHealth healthPkt;
		healthPkt.health = getHeartsHealth();
		healthPkt.Serialize(session->stream);

		if (getHeartsHealth() - lastHealth < 0) {
			Packet::EntityEvent pkt;
			pkt.entityId = this->id;
			pkt.action = PacketData::EntityEvent::HURT;
			pkt.Serialize(session->stream);

			// Let other players know
			session->entityTracker->SendPacketToViewers(pkt, this->id);
		}

		this->lastHealth = getHeartsHealth();
	}

	// If we fell out of the world then die
	if (position.y < -64.0)
		OnDeath();

	// Tell entities we collided with a player
	if (entityManager) {
		auto entitiesCollidingWith = entityManager->GetEntitiesWithinAabbExcluding(collider.Expand(1.0, 0.0, 1.0),
		                                                                           this->id);
		for (const auto& entity : entitiesCollidingWith) {
			if (!entity->isDead)
				entity->OnCollideWithPlayer(*this);
		}
	}
}