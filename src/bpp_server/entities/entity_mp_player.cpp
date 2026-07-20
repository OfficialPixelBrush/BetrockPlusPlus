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

bool EntityMPPlayer::pickupItem(ItemStack& stack, EntityId entityId) {
	if (this->session->inventory.pickupItem(stack)) {
		Packet::CollectItem pkt;
		pkt.collector_entity_id = this->id;
		pkt.item_entity_id = entityId;
		this->session->entityTracker->sendPacketToViewers(pkt, this->id);
		pkt.Serialize(this->session->stream);
		return true;
	}

	return false;
}

// This works over a copy of your item, it doesn't remove or decrement it !!!
bool EntityMPPlayer::dropItem(ItemStack stack) {
	if (stack.id == Items::Id::INVALID || stack.count <= 0)
		return false;

	// Create the item entity
	Vec3 itemPos = { position.x, position.y - 0.3 + PLAYER_EYE_HEIGHT, position.z };
	std::shared_ptr<ItemEntity> itemEntity = std::make_shared<ItemEntity>(itemPos);
	itemEntity->itemStack = stack;
	itemEntity->pickupCooldown = 40; // So we don't pick it up instantly
	itemEntity->dim = dim;

	// Give ourselves some random velocity based on look direction
	float velocity = 0.3f;
	itemEntity->velocity.x = double(-std::sin(this->rotationYaw / 180.0F * JavaMath::PI_FLOAT) *
	                                std::cos(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * velocity);
	itemEntity->velocity.z = double(std::cos(this->rotationYaw / 180.0F * JavaMath::PI_FLOAT) *
	                                std::cos(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * velocity);
	itemEntity->velocity.y = double(-std::sin(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * velocity + 0.1F);

	// Add a little bit of randomness
	velocity = 0.02f;
	float angle = rand.nextFloat() * JavaMath::PI_FLOAT * 2.0f;
	velocity *= rand.nextFloat();
	itemEntity->velocity.x += std::cos(angle) * velocity;
	itemEntity->velocity.y += (rand.nextFloat() - rand.nextFloat()) * 0.1f;
	itemEntity->velocity.z += std::sin(angle) * velocity;

	// Register our item with the world
	this->world->entityManager.addEntity(std::move(itemEntity));
	return true;
}

void EntityMPPlayer::handlePositionChecks() {
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
			this->teleport({ ptp.x, ptp.y, ptp.z }, { rotationYaw, rotationPitch });
			session->position.pos = *session->pendingTeleport;
			Packet::PlayerPosition pkt;
			pkt.on_ground = onGround;
			pkt.position = { position.x, position.y, position.z };
			pkt.camera_y = position.y; // This is backwards, thanks notch
			pkt.Serialize(session->stream);
			return;
		}
		// Client acknowledged our tp
		session->pendingTeleport.reset();
	}

	// Always trust rotations
	this->rotationYaw = session->rotation.x;
	this->rotationPitch = session->rotation.y;

	// If we recieved a movement packet this tick do our server side checks
	if (session->pendingPosition) {
		// Re-simulate our move
		bool residualTooLarge = false;
		bool movedWrong = false;
		bool wasClearBefore = world->getCollidingBoundingBoxes(collider.expand(-0.0625, -0.0625, -0.0625)).empty();
		Vec3 lastPosition = this->position;
		Vec3 claimed = *session->pendingPosition;
		Vec3 delta = claimed - lastPosition;
		if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z > 100.0) {
			GlobalLogger().warn << "Client " << session->username << " moved wrongly!\n";
			movedWrong = true;
		}
		move(delta);

		// How far is our simulated move vs what the client says?
		delta = claimed - this->position;
		double residual = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
		if (residual < 0.0625) {
			this->position = claimed; // Trust it
			session->position.pos = claimed;
			rebuildCollider();
		} else {
			// Send a correction
			residualTooLarge = true;
		}

		bool clearNow = world->getCollidingBoundingBoxes(collider.expand(-0.0625, -0.0625, -0.0625)).empty();

		if ((wasClearBefore && (residualTooLarge || !clearNow)) || movedWrong) {
			// TP our player back
			GlobalLogger().info << "Residual from move was: " << residual << "\n";
			GlobalLogger().info << "Rubberbanded player! wasClear=" << wasClearBefore
			                      << " residual=" << residualTooLarge << " clearNow=" << clearNow
			                      << " movedWrong=" << movedWrong << " pos=(" << this->position.x << ","
			                      << this->position.y << "," << this->position.z << ")"
			                      << " colliderY=[" << collider.minY << "," << collider.maxY << "]"
			                      << "\n";
			this->teleport(lastPosition, { rotationYaw, rotationPitch });
			session->position.pos = lastPosition;
			Packet::PlayerPosition pkt;
			pkt.on_ground = onGround;
			pkt.position = { position.x, position.y + PLAYER_EYE_HEIGHT + 0.0625, position.z };
			pkt.camera_y = position.y; // This is backwards, thanks notch
			pkt.Serialize(session->stream);
		}

		session->pendingPosition.reset();
	}
}

void EntityMPPlayer::tick() {
	if (!session)
		return;

	// Handle reported vs server side position
	this->handlePositionChecks();

	// Do living entity stuff
	MobileEntity::tick();

	// Tell entities we collided with a player
	if (entityManager) {
		auto entitiesCollidingWith = entityManager->getEntitiesWithinAABBExcluding(collider.expand(1.0, 0.0, 1.0), this->id);
		for (const auto& entity : entitiesCollidingWith) {
			if (!entity->isDead)
				entity->onCollideWithPlayer(*this);
		}
	}
}