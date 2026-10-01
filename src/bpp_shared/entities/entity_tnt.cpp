/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_tnt.h"

static Tag SetByte(std::string _name, int8_t _value) {
	Tag byteTag;
	byteTag.type = TAG_BYTE;
	byteTag.name = _name;
	byteTag.byteValue = _value;

	return byteTag;
}

void TntEntity::Tick() {
	if (!world)
		return;

	this->velocity.y -= 0.04;
	this->Move(this->velocity);
	this->velocity *= 0.98;
	if (this->onGround) {
		this->velocity.x *= 0.7;
		this->velocity.y *= -0.5;
		this->velocity.z *= 0.7;
	}

	if (this->fuse-- <= 0) {
		this->isDead = true;
		this->Explode();
	}
}

std::optional<Tag> TntEntity::SerializeToNbt() {
	auto tag = Entity::SerializeToNbt();
	if (!tag)
		return std::nullopt;

	tag->compound["Fuse"] = SetByte("Fuse", this->fuse);

	return tag;
}

void TntEntity::LoadFromNbt(Tag& _nbt) {
	Entity::LoadFromNbt(_nbt);

	this->fuse = _nbt.Get("Fuse").byteValue;
}