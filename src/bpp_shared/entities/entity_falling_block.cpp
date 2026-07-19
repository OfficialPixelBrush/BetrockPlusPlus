/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_falling_block.h"
#include "entities/entity_item.h"
#include "java_math.h"
#include "world/world.h"

void FallingBlockEntity::tick() {
	if (m_block == BLOCK_AIR) {
		m_isDead = true;
		return;
	}

	m_ticksFallen++;
	m_velocity.m_y -= 0.04;
	move(this->m_velocity);
	m_velocity *= { 0.98, 0.98, 0.98 };

	auto fd = MathHelper::floor_double;
	Int3 blockPosition = { fd(m_position.m_x), fd(m_position.m_y), fd(m_position.m_z) };

	if (m_onGround) {
		m_velocity *= { 0.7, -0.5, 0.7 };
		m_isDead = true;
		// TODO: check if we can actually fall here properly
		this->m_world->setBlock(blockPosition, this->m_block, 0);
		return;
	}
	if (m_ticksFallen > 100) {
		// Create the item entity
		Vec3 itemPos = m_position;
		std::shared_ptr<ItemEntity> itemEntity = std::make_shared<ItemEntity>(itemPos);
		itemEntity->m_itemStack = { this->m_block, 1 };
		itemEntity->m_dim = m_dim;

		// Register our item with the world
		this->m_world->m_entityManager.addEntity(std::move(itemEntity));
		m_isDead = true;
	}
}