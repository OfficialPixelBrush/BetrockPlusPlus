/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "entity_mp_player.h"
#include "entities/entity_item.h"
#include "../player_conn/player_session.h"
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
}

bool EntityMPPlayer::dropHeldItem(int count) {
	auto itemStack = this->session->inventory.getHeldItem();
	if (!itemStack) return false;
	if (itemStack->id == ITEM_INVALID || itemStack->count <= 0) return false;

	int difference = itemStack->count - count;
	int newSize = std::max(0, difference);
	int droppedSize = count;
	if (difference < 0) droppedSize -= difference;

	itemStack->count = newSize;

	// Create our new item stack
	ItemStack newStack;
	newStack.count = droppedSize;
	newStack.data = itemStack->data;
	newStack.id = itemStack->id;

	// Create the item entity
	Vec3 position = { posX, posY - 0.3 + PLAYER_EYE_HEIGHT, posZ };
	std::shared_ptr<ItemEntity> itemEntity = std::make_shared<ItemEntity>(position);
	itemEntity->itemStack = std::move(newStack);
	itemEntity->pickupCooldown = 40; // So we don't pick it up instantly

	// If we exhausted all our held item then clear it
	if (itemStack->count <= 0) {
		itemStack->id = ITEM_INVALID;
		itemStack->count = 0;
		itemStack->data = 0;
	}

	// Give ourselves some random velocity based on look direction
	float velocity = 0.4f;
	itemEntity->motionX = double(-std::sin(this->rotationYaw   / 180.0F * JavaMath::PI_FLOAT) * std::cos(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * velocity);
	itemEntity->motionZ = double( std::cos(this->rotationYaw   / 180.0F * JavaMath::PI_FLOAT) * std::cos(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * velocity);
	itemEntity->motionY = double(-std::sin(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * velocity + 0.1F);

	// Add a little bit of randomness
	velocity = 0.02f;
	float angle = rand.nextFloat() * JavaMath::PI_FLOAT * 2.0f;
	velocity *= rand.nextFloat();
	itemEntity->motionX += std::cos(angle) * velocity;
	itemEntity->motionY += (rand.nextFloat() - rand.nextFloat()) * 0.1f;
	itemEntity->motionZ += std::sin(angle) * velocity;

	// Register our item with the world
	this->world->entityManager.addEntity(std::move(itemEntity));
	return true;
}

// We ignore physics for the player entity and just grab what the client tells us
void EntityMPPlayer::tick() {
	if (!session)
		return;
	Vec3 claimed = session->position.pos;

	posX = claimed.x;
	posY = claimed.y;
	posZ = claimed.z;
	rotationYaw = session->rotation.x;
	rotationPitch = session->rotation.y;

	rebuildCollider();

	// Tell entities we collided with them
	if (entityManager) {
		AABB colliderCopy = collider.expand(1.0, 0.0, 1.0);
		auto entitiesCollidingWith = entityManager->getEntitiesWithinAABBExcluding(colliderCopy, this->id);
		for (auto& entity : entitiesCollidingWith) {
			if (entity->collider.intersects(colliderCopy) && !entity->isDead)
				entity->onCollideWithPlayer(*this);
		}
	}
}