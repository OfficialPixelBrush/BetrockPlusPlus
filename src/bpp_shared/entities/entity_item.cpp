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

void ItemEntity::LoadFromNbt(Tag& _nbt) {
	Entity::LoadFromNbt(_nbt);

	// Load item specific stuff
	this->health = _nbt.compound["Health"].GetShort();
	this->ticksExisted = _nbt.compound["Age"].GetShort();

	// Load our item
	const auto& itemCompound = _nbt.compound["Item"].GetCompound();
	this->itemStack = { itemCompound.at("id").GetShort(), itemCompound.at("Count").GetByte(),
		                itemCompound.at("Damage").GetShort() };
}

std::optional<Tag> ItemEntity::SerializeToNbt() {
	auto tag = Entity::SerializeToNbt();
	if (!tag)
		return std::nullopt;

	// Our additions
	Tag health;
	health.name = "Health";
	health.type = TAG_SHORT;
	health.shortValue = this->health;
	Tag age;
	age.name = "Age";
	age.type = TAG_SHORT;
	age.shortValue = this->ticksExisted;

	// Construct the item nbt
	Tag item;
	item.name = "Item";
	item.type = TAG_COMPOUND;
	Tag id;
	id.name = "id";
	id.type = TAG_SHORT;
	id.shortValue = this->itemStack.id;
	Tag count;
	count.name = "Count";
	count.type = TAG_BYTE;
	count.byteValue = this->itemStack.count;
	Tag damage;
	damage.name = "Damage";
	damage.type = TAG_SHORT;
	damage.shortValue = this->itemStack.data;
	item.compound["id"] = id;
	item.compound["Count"] = count;
	item.compound["Damage"] = damage;

	// Add our additions to the base tag
	tag->compound["Health"] = health;
	tag->compound["Age"] = age;
	tag->compound["Item"] = item;

	return tag;
}

void ItemEntity::OnCollideWithPlayer(PlayerEntity& _entity) {
	if (pickupCooldown)
		return;
	_entity.PickupItem(this->itemStack, this->id);
	if (this->itemStack.count <= 0)
		this->isDead = true;
}

void ItemEntity::Tick() {
	// Item entities have differing physics
	Entity::Tick();
	pickupCooldown--;
	pickupCooldown = std::max(int8_t(0), pickupCooldown);

	velocity.y -= 0.04;

	if (world->GetMaterial({ MathHelper::FloorDouble(position.x), MathHelper::FloorDouble(position.y),
	                         MathHelper::FloorDouble(position.z) }) == Material::Lava()) {
		velocity.y = 0.2;
		velocity.x = double((rand.NextFloat() - rand.NextFloat()) * 0.2F);
		velocity.z = double((rand.NextFloat() - rand.NextFloat()) * 0.2F);
	}

	PushOutOfBlocks({ position.x, (collider.minY + collider.maxY) / 2.0, position.z });
	Move(this->velocity);

	float horizontalDrag = 0.98f;
	if (onGround) {
		horizontalDrag = 0.58800006f;

		// Look up the block below us
		int bx = MathHelper::FloorDouble(position.x);
		int by = MathHelper::FloorDouble(collider.minY) - 1;
		int bz = MathHelper::FloorDouble(position.z);
		int blockId = world->GetBlockId({ bx, by, bz });
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