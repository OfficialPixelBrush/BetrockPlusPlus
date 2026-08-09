/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_player.h"
#include "entity_item.h"

bool PlayerEntity::PickupItem(ItemStack& _stack, EntityId _entityId) {
	return true;
}

void PlayerEntity::DropInventory() {
	return;
}
void PlayerEntity::OnDeath() {
	MobileEntity::OnDeath();

	// Shrink to the "squished corpse" hitbox 
	width = 0.2f;
	height = 0.2f;
	RebuildCollider();

	velocity.y = 0.1;

	// Fling away from whatever hit us
	velocity.x = double(-std::cos((attackedAtYaw + rotationYaw) * JavaMath::PI / 180.0f) * 0.1f);
	velocity.z = double(-std::sin((attackedAtYaw + rotationYaw) * JavaMath::PI / 180.0f) * 0.1f);

	yOffset = 0.1f;

	this->forceVelocityUpdate = true;

	this->DropInventory();
}

bool PlayerEntity::DropItem(ItemStack _stack) {
	if (_stack.id == Items::Id::INVALID || _stack.count <= 0)
		return false;

	// Create the item entity
	Vec3 itemPos = { position.x, position.y - 0.3 + PLAYER_EYE_HEIGHT, position.z };
	std::shared_ptr<ItemEntity> itemEntity = std::make_shared<ItemEntity>(itemPos);
	itemEntity->itemStack = _stack;
	itemEntity->pickupCooldown = 40; // So we don't pick it up instantly

	// Give ourselves some random velocity based on look direction
	float initVelocity = 0.3f;
	itemEntity->velocity.x = double(-std::sin(this->rotationYaw / 180.0F * JavaMath::PI_FLOAT) *
	                                std::cos(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * initVelocity);
	itemEntity->velocity.z = double(std::cos(this->rotationYaw / 180.0F * JavaMath::PI_FLOAT) *
	                                std::cos(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * initVelocity);
	itemEntity->velocity.y = double(-std::sin(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * initVelocity + 0.1F);

	// Add a little bit of randomness
	initVelocity = 0.02f;
	float angle = rand.NextFloat() * JavaMath::PI_FLOAT * 2.0f;
	initVelocity *= rand.NextFloat();
	itemEntity->velocity.x += std::cos(angle) * initVelocity;
	itemEntity->velocity.y += (rand.NextFloat() - rand.NextFloat()) * 0.1f;
	itemEntity->velocity.z += std::sin(angle) * initVelocity;

	// Register our item with the world
	this->world->entityManager.AddEntity(std::move(itemEntity));
	return true;
}