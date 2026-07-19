/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_item.h"
#include "entity_player.h"
#include "world/world.h"
#include <algorithm>
#include <cmath>

void ItemEntity::loadFromNBT(Tag& nbt) {
	Entity::loadFromNBT(nbt);

	// Load item specific stuff
	this->m_health = nbt.m_compound["Health"].getShort();
	this->m_ticksExisted = nbt.m_compound["Age"].getShort();

	// Load our item
	const auto& itemCompound = nbt.m_compound["Item"].getCompound();
	this->m_itemStack = { itemCompound.at("id").getShort(), itemCompound.at("Count").getByte(),
		                itemCompound.at("Damage").getShort() };
}

std::optional<Tag> ItemEntity::serializeToNBT() {
	auto tag = Entity::serializeToNBT();
	if (!tag)
		return std::nullopt;

	// Our additions
	Tag Health;
	Health.m_name = "Health";
	Health.m_type = TAG_SHORT;
	Health.m_shortValue = this->m_health;
	Tag Age;
	Age.m_name = "Age";
	Age.m_type = TAG_SHORT;
	Age.m_shortValue = this->m_ticksExisted;

	// Construct the item nbt
	Tag Item;
	Item.m_name = "Item";
	Item.m_type = TAG_COMPOUND;
	Tag id;
	id.m_name = "id";
	id.m_type = TAG_SHORT;
	id.m_shortValue = this->m_itemStack.m_id;
	Tag Count;
	Count.m_name = "Count";
	Count.m_type = TAG_BYTE;
	Count.m_byteValue = this->m_itemStack.m_count;
	Tag Damage;
	Damage.m_name = "Damage";
	Damage.m_type = TAG_SHORT;
	Damage.m_shortValue = this->m_itemStack.m_data;
	Item.m_compound["id"] = id;
	Item.m_compound["Count"] = Count;
	Item.m_compound["Damage"] = Damage;

	// Add our additions to the base tag
	tag->m_compound["Health"] = Health;
	tag->m_compound["Age"] = Age;
	tag->m_compound["Item"] = Item;

	return tag;
}

void ItemEntity::onCollideWithPlayer(PlayerEntity& entity) {
	if (m_pickupCooldown)
		return;
	entity.pickupItem(this->m_itemStack, this->m_id);
	if (this->m_itemStack.m_count <= 0)
		this->m_isDead = true;
}

void ItemEntity::tick() {
	// Item entities have differing physics
	Entity::tick();
	m_pickupCooldown--;
	m_pickupCooldown = std::max(int8_t(0), m_pickupCooldown);

	m_velocity.m_y -= 0.04;

	if (m_world->getMaterial({ MathHelper::floor_double(m_position.m_x), MathHelper::floor_double(m_position.m_y),
	                         MathHelper::floor_double(m_position.m_z) }) == Material::Lava()) {
		m_velocity.m_y = 0.2;
		m_velocity.m_x = double((m_rand.nextFloat() - m_rand.nextFloat()) * 0.2F);
		m_velocity.m_z = double((m_rand.nextFloat() - m_rand.nextFloat()) * 0.2F);
	}

	pushOutOfBlocks({ m_position.m_x, (m_collider.m_minY + m_collider.m_maxY) / 2.0, m_position.m_z });
	move(this->m_velocity);

	float horizontalDrag = 0.98f;
	if (m_onGround) {
		horizontalDrag = 0.58800006f;

		// Look up the block below us
		int bx = MathHelper::floor_double(m_position.m_x);
		int by = MathHelper::floor_double(m_collider.m_minY) - 1;
		int bz = MathHelper::floor_double(m_position.m_z);
		int blockId = m_world->getBlockId({ bx, by, bz });
		m_belowBlock = Blocks::blockProperties[blockId];

		if (blockId > 0)
			horizontalDrag = m_belowBlock.m_slipperiness * 0.98f;
	}

	m_velocity.m_x *= double(horizontalDrag);
	m_velocity.m_y *= 0.9800000190734863;
	m_velocity.m_z *= double(horizontalDrag);

	// Bounce when we land
	if (m_onGround)
		m_velocity.m_y *= -0.5;

	if (m_ticksExisted >= 6000)
		m_isDead = true;
}