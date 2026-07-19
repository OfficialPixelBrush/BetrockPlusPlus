/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity.h"

struct FallingBlockEntity : public Entity {
	BlockType m_block = BLOCK_AIR;
	int m_ticksFallen = 0;

	FallingBlockEntity(Vec3 spawnPosition, BlockType block) : Entity() {
		this->m_block = block;
		this->m_position = spawnPosition;
		this->m_height = 0.98;
		this->m_width = 0.98;
		this->m_yOffset = m_height / 2.0f;
		this->m_preventEntitySpawning = true;
		rebuildCollider();

		if (block == BLOCK_SAND)
			m_type = EntityType::FALLING_SAND;
		if (block == BLOCK_GRAVEL)
			m_type = EntityType::FALLING_GRAVEL;
	}

	void tick() override;
	std::optional<Tag> serializeToNBT() override {
		auto root = Entity::serializeToNBT();
		if (!root || this->m_block == BLOCK_AIR)
			return std::nullopt;
		Tag Tile;
		Tile.m_type = TAG_BYTE;
		Tile.m_name = "Tile";
		Tile.m_byteValue = this->m_block;
		root->m_compound["Tile"] = Tile;
		return root;
	}
	void loadFromNBT(Tag& nbt) override {
		Entity::loadFromNBT(nbt);
		this->m_block = BlockType(nbt.m_compound["Tile"].getByte() & 255);
	}
};