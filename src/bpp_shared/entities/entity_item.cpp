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
	this->health = nbt.compound["Health"].getShort();
	this->ticksExisted = nbt.compound["Age"].getShort();

	// Load our item
	const auto& itemCompound = nbt.compound["Item"].getCompound();
	this->itemStack = { itemCompound.at("id").getShort(), itemCompound.at("Count").getByte(),
		                itemCompound.at("Damage").getShort() };
}

std::optional<Tag> ItemEntity::serializeToNBT() {
	auto tag = Entity::serializeToNBT();
	if (!tag)
		return std::nullopt;

	// Our additions
	Tag Health;
	Health.name = "Health";
	Health.type = TAG_SHORT;
	Health.shortValue = this->health;
	Tag Age;
	Age.name = "Age";
	Age.type = TAG_SHORT;
	Age.shortValue = this->ticksExisted;

	// Construct the item nbt
	Tag Item;
	Item.name = "Item";
	Item.type = TAG_COMPOUND;
	Tag id;
	id.name = "id";
	id.type = TAG_SHORT;
	id.shortValue = this->itemStack.id;
	Tag Count;
	Count.name = "Count";
	Count.type = TAG_BYTE;
	Count.byteValue = this->itemStack.count;
	Tag Damage;
	Damage.name = "Damage";
	Damage.type = TAG_SHORT;
	Damage.shortValue = this->itemStack.data;
	Item.compound["id"] = id;
	Item.compound["Count"] = Count;
	Item.compound["Damage"] = Damage;

	// Add our additions to the base tag
	tag->compound["Health"] = Health;
	tag->compound["Age"] = Age;
	tag->compound["Item"] = Item;

	return tag;
}

void ItemEntity::onCollideWithPlayer(PlayerEntity& entity) {
	if (pickupCooldown)
		return;
	entity.pickupItem(this->itemStack, this->id);
	if (this->itemStack.count <= 0)
		this->isDead = true;
}

void ItemEntity::tick() {
	// Item entities have differing physics
	Entity::tick();
	pickupCooldown--;
	pickupCooldown = std::max(int8_t(0), pickupCooldown);

	velocity.y -= 0.04;

	if (world->getMaterial({ MathHelper::floor_double(position.x), MathHelper::floor_double(position.y),
	                         MathHelper::floor_double(position.z) }) == Material::Lava()) {
		velocity.y = 0.2;
		velocity.x = double((rand.nextFloat() - rand.nextFloat()) * 0.2F);
		velocity.z = double((rand.nextFloat() - rand.nextFloat()) * 0.2F);
	}

	pushOutOfBlocks({ position.x, (collider.minY + collider.maxY) / 2.0, position.z });
	move(this->velocity);

	float horizontalDrag = 0.98f;
	if (onGround) {
		horizontalDrag = 0.58800006f;

		// Look up the block below us
		int bx = MathHelper::floor_double(position.x);
		int by = MathHelper::floor_double(collider.minY) - 1;
		int bz = MathHelper::floor_double(position.z);
		int blockId = world->getBlockId({ bx, by, bz });
		belowBlock = Blocks::blockProperties[blockId];

		if (blockId > 0)
			horizontalDrag = belowBlock.slipperiness * 0.98f;
	}

	velocity.x *= double(horizontalDrag);
	velocity.y *= 0.9800000190734863;
	velocity.z *= double(horizontalDrag);

	// Bounce when we land
	if (onGround)
		velocity.y *= -0.5;

	if (ticksExisted >= 6000)
		isDead = true;
}