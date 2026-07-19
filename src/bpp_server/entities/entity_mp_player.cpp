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
	if (this->m_session->m_inventory.pickupItem(stack)) {
		Packet::CollectItem pkt;
		pkt.m_collector_entity_id = this->m_id;
		pkt.m_item_entity_id = entityId;
		this->m_session->m_entityTracker->sendPacketToViewers(pkt, this->m_id);
		pkt.Serialize(this->m_session->m_stream);
		return true;
	}

	return false;
}

// This works over a copy of your item, it doesn't remove or decrement it !!!
bool EntityMPPlayer::dropItem(ItemStack stack) {
	if (stack.m_id == Items::Id::INVALID || stack.m_count <= 0)
		return false;

	// Create the item entity
	Vec3 itemPos = { m_position.m_x, m_position.m_y - 0.3 + PLAYER_EYE_HEIGHT, m_position.m_z };
	std::shared_ptr<ItemEntity> itemEntity = std::make_shared<ItemEntity>(itemPos);
	itemEntity->m_itemStack = stack;
	itemEntity->m_pickupCooldown = 40; // So we don't pick it up instantly
	itemEntity->m_dim = m_dim;

	// Give ourselves some random velocity based on look direction
	float velocity = 0.3f;
	itemEntity->m_velocity.m_x = double(-std::sin(this->m_rotationYaw / 180.0F * JavaMath::PI_FLOAT) *
	                                std::cos(this->m_rotationPitch / 180.0F * JavaMath::PI_FLOAT) * velocity);
	itemEntity->m_velocity.m_z = double(std::cos(this->m_rotationYaw / 180.0F * JavaMath::PI_FLOAT) *
	                                std::cos(this->m_rotationPitch / 180.0F * JavaMath::PI_FLOAT) * velocity);
	itemEntity->m_velocity.m_y = double(-std::sin(this->m_rotationPitch / 180.0F * JavaMath::PI_FLOAT) * velocity + 0.1F);

	// Add a little bit of randomness
	velocity = 0.02f;
	float angle = m_rand.nextFloat() * JavaMath::PI_FLOAT * 2.0f;
	velocity *= m_rand.nextFloat();
	itemEntity->m_velocity.m_x += std::cos(angle) * velocity;
	itemEntity->m_velocity.m_y += (m_rand.nextFloat() - m_rand.nextFloat()) * 0.1f;
	itemEntity->m_velocity.m_z += std::sin(angle) * velocity;

	// Register our item with the world
	this->m_world->m_entityManager.addEntity(std::move(itemEntity));
	return true;
}

void EntityMPPlayer::tick() {
	if (!m_session)
		return;
	if (m_session->m_pendingTeleport) {
		// We have a pending teleport. Check to see if the player caught up
		if (!m_session->m_pendingPosition)
			return;
		auto& pending = *m_session->m_pendingPosition;
		Vec3 claimed = { pending.m_x, pending.m_y + PLAYER_EYE_HEIGHT, pending.m_z };
		Vec3 delta = claimed - *m_session->m_pendingTeleport;
		auto dist = delta.m_x * delta.m_x + delta.m_y * delta.m_y + delta.m_z * delta.m_z;
		if (dist > 0.0625) {
			// Player isn't at the teleported position so send another tp packet
			// Also reset our position
			auto ptp = *m_session->m_pendingTeleport;
			this->teleport({ ptp.m_x, ptp.m_y + 1.0 / 64.0, ptp.m_z}, { m_rotationYaw, m_rotationPitch });
			m_session->m_position.m_pos = *m_session->m_pendingTeleport;
			Packet::PlayerPosition pkt;
			pkt.m_on_ground = m_onGround;
			pkt.m_position = m_position;
			pkt.Serialize(m_session->m_stream);
			return;
		}
		// Client acknowledged our tp
		m_session->m_pendingTeleport.reset();
	}

	// Do living entity stuff
	MobileEntity::tick();

	// Always trust rotations
	this->m_rotationYaw = m_session->m_rotation.m_x;
	this->m_rotationPitch = m_session->m_rotation.m_y;

	// If we recieved a movement packet this tick do our server side checks
	if (m_session->m_pendingPosition) {
		m_session->m_pendingPosition.reset();

		// Re-simulate our move
		bool residualTooLarge = false;
		bool movedWrong = false;
		bool wasClearBefore = m_world->getCollidingBoundingBoxes(m_collider.expand(-0.0625, -0.0625, -0.0625)).empty();
		Vec3 lastPosition = this->m_position;
		Vec3 claimed = *m_session->m_pendingPosition;
		Vec3 delta = claimed - lastPosition;
		if (delta.m_x * delta.m_x + delta.m_y * delta.m_y + delta.m_z * delta.m_z > 100.0) {
			GlobalLogger().m_warn << "Client " << m_session->m_username << " moved wrongly!\n";
			movedWrong = true;
		}
		move(delta);

		// How far is our simulated move vs what the client says?
		delta = claimed - this->m_position;
		double residual = delta.m_x * delta.m_x + delta.m_y * delta.m_y + delta.m_z * delta.m_z;
		if (residual < 0.0625) {
			this->m_position = claimed; // Trust it
			m_session->m_position.m_pos = claimed;
			rebuildCollider();
		} else {
			// Send a correction
			residualTooLarge = true;
		}

		bool clearNow = m_world->getCollidingBoundingBoxes(m_collider.expand(-0.0625, -0.0625, -0.0625)).empty();

		if ((wasClearBefore && (residualTooLarge || !clearNow)) || movedWrong) {
			// TP our player back
			GlobalLogger().m_info << "Residual from move was: " << residual << "\n";
			GlobalLogger().m_info << "Rubberbanded player! wasClear=" << wasClearBefore
			                    << " residual=" << residualTooLarge << " clearNow=" << clearNow
			                    << " movedWrong=" << movedWrong << " pos=(" << this->m_position.m_x << ","
			                    << this->m_position.m_y << "," << this->m_position.m_z << ")"
			                    << " colliderY=[" << m_collider.m_minY << "," << m_collider.m_maxY << "]"
			                    << "\n";
			this->teleport(lastPosition, { m_rotationYaw, m_rotationPitch });
			m_session->m_position.m_pos = lastPosition;
			Packet::PlayerPosition pkt;
			pkt.m_on_ground = m_onGround;
			pkt.m_position = { m_position.m_x, m_position.m_y + PLAYER_EYE_HEIGHT + 1.0 / 64.0, m_position.m_z };
			pkt.m_camera_y = m_position.m_y; // This is backwards, thanks notch
			pkt.Serialize(m_session->m_stream);
		}
	}

	// Tell entities we collided with a player
	if (m_entityManager) {
		auto entitiesCollidingWith = m_entityManager->getEntitiesWithinAABBExcluding(m_collider.expand(1.0, 0.0, 1.0), this->m_id);
		for (const auto& entity : entitiesCollidingWith) {
			if (!entity->m_isDead)
				entity->onCollideWithPlayer(*this);
		}
	}
}