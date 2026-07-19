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
	ItemStack m_itemStack;
	int8_t m_health = 5;
	int8_t m_pickupCooldown = 10;
	ItemEntity(Vec3 position) : Entity() {
		m_type = EntityType::ITEM;
		m_width = 0.25f;
		m_height = 0.25f;
		m_yOffset = 0.125f; // height / 2
		m_stepHeight = 0.0f;

		// Set the initial position of the item entity
		this->teleport(position);

		// This stuff is mostly randomized
		m_rotationYaw = m_rand.nextDouble() * 360.0;
		m_velocity.m_x = m_rand.nextDouble() * 0.2 - 0.1;
		m_velocity.m_y = 0.2;
		m_velocity.m_z = m_rand.nextDouble() * 0.2 - 0.1;
	}

	AABB getFluidCollider() override {
		// Returns the collider we use to compare if we are in a fluid
		return m_collider;
	}
	AABB getLavaCollider() override {
		// Returns the collider we use to detect if we are in lava
		return m_collider;
	}
	void onCollideWithPlayer(PlayerEntity& entity) override;
	void tick() override;
	std::optional<Tag> serializeToNBT() override;
	void loadFromNBT(Tag& nbt) override;
	bool attackEntityFrom(Entity* entity, int damage) override {
		Entity::attackEntityFrom(entity, damage);
		m_health -= damage;
		if (m_health <= 0)
			m_isDead = true;
		return false;
	}
};