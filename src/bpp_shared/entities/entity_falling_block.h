/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity.h"

struct FallingBlockEntity : public Entity {
	BlockType block = BLOCK_AIR;
	int ticksFallen = 0;

	FallingBlockEntity(Vec3 _spawnPosition, BlockType _block) : Entity() {
		this->block = _block;
		this->position = _spawnPosition;
		this->height = 0.98;
		this->width = 0.98;
		this->yOffset = height / 2.0f;
		this->preventEntitySpawning = true;
		rebuildCollider();

		if (_block == BLOCK_SAND)
			type = EntityType::FALLING_SAND;
		if (_block == BLOCK_GRAVEL)
			type = EntityType::FALLING_GRAVEL;
	}

	void tick() override;
	std::optional<Tag> serializeToNBT() override {
		auto root = Entity::serializeToNBT();
		if (!root || this->block == BLOCK_AIR)
			return std::nullopt;
		Tag Tile;
		Tile.type = TAG_BYTE;
		Tile.name = "Tile";
		Tile.byteValue = this->block;
		root->compound["Tile"] = Tile;
		return root;
	}
	void loadFromNBT(Tag& _nbt) override {
		Entity::loadFromNBT(_nbt);
		this->block = BlockType(_nbt.compound["Tile"].getByte() & 255);
	}
};