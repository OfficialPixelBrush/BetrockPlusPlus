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

// We ignore physics for the player entity and just grab what the client tells us
void EntityMPPlayer::tick() {
	if (!session)
		return;
	Vec3 claimed = session->position.pos;

	position.x = claimed.x;
	position.y = claimed.y;
	position.z = claimed.z;
	rotationYaw = session->rotation.x;
	rotationPitch = session->rotation.y;

	rebuildCollider();

	// Tell entities we collided with them
	if (entityManager) {
		auto entitiesCollidingWith = entityManager->getEntitiesWithinAABBExcluding(collider.expand(1.0, 0.0, 1.0),
		                                                                           this->id);
		for (const auto& entity : entitiesCollidingWith) {
			if (!entity->isDead)
				entity->onCollideWithPlayer(*this);
		}
	}
}