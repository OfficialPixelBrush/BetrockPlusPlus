/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_sheep.h"

void SheepEntity::OnDeath(Entity* _killer) {
	if (!isSheared)
		DropItemAtEntity(BLOCK_WOOL, 1, this->color);
}

void SheepEntity::EncodeMetadata(std::vector<PacketData::EntityMetadata::DataEntry>& _metadata) {
    Entity::EncodeMetadata(_metadata);

	const int8_t value =
		static_cast<int8_t>(
			(color & 0x0F) |
			(isSheared ? 0x10 : 0)
		);

	_metadata.push_back({
		.type = PacketData::EntityMetadata::BYTE,
		.index = 16,
		.value = value
	});
}

bool SheepEntity::DecodeMetadata(const std::vector<PacketData::EntityMetadata::DataEntry>& _metadata) {
    if (!Entity::DecodeMetadata(_metadata)) {
        return false;
    }

	if (auto* raw = FindMetadata<int8_t>(_metadata, PacketData::EntityMetadata::BYTE, 16)) {
        isSheared = (*raw & 0x10) != 0;
        color = *raw & 0x0F;
        return true;
	}
	return false;
}