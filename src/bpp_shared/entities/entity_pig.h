/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_animal.h"

struct PigEntity : public AnimalEntity {
	bool isSaddled = false;
	PigEntity() : AnimalEntity() {
		type = EntityType::PIG;
		width = 0.9f;
		height = 0.9f;
	}
	~PigEntity() = default;
	bool TryDespawn() override;
	void OnDeath(Entity* _killer) override;
	void OnPlayerInteract(PlayerEntity* _entity) override;
	void EncodeMetadata(std::vector<PacketData::EntityMetadata::DataEntry>& _metadata) override;
	bool DecodeMetadata(const std::vector<PacketData::EntityMetadata::DataEntry>& _metadata) override;
	std::optional<Tag> SerializeToNbt() override;
	void LoadFromNbt(Tag& _nbt) override;
};