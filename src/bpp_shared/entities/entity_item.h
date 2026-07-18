/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include "entity.h"
#include "inventory/item_stack.h"
#include "logger.h"

struct ItemEntity : public Entity {
	ItemStack itemStack;
	int8_t health = 5;
	int8_t pickupCooldown = 10;
	ItemEntity(Vec3 position) : Entity() {
		type = EntityType::ITEM;
		width = 0.25f;
		height = 0.25f;
		yOffset = 0.125f; // height / 2

		// Set the initial position of the item entity
		this->teleport(position);

		// This stuff is mostly randomized
		rotationYaw = rand.nextDouble() * 360.0;
		velocity.x = rand.nextDouble() * 0.2 - 0.1;
		velocity.y = 0.2;
		velocity.z = rand.nextDouble() * 0.2 - 0.1;
	}

	AABB getFluidCollider() override {
		// Returns the collider we use to compare if we are in a fluid
		return collider;
	}
	AABB getLavaCollider() override {
		// Returns the collider we use to detect if we are in lava
		return collider;
	}
	void onCollideWithPlayer(PlayerEntity& entity) override;
	void tick() override;
	std::optional<Tag> serializeToNBT() override;
	void loadFromNBT(Tag& nbt) override;
	bool attackEntityFrom(Entity* entity, int damage) override {
		Entity::attackEntityFrom(entity, damage);
		health -= damage;
		if (health <= 0)
			isDead = true;
		return false;
	}
};