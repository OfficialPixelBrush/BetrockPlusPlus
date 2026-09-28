/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_slime.h"

void SlimeEntity::SplitSlime() {
	for (int i = 0; i < 4; i++) {
		float xOffset = (float(i % 2) - 0.5f) * this->size / 4.0f;
		float zOffset = (float(i / 2) - 0.5f) * this->size / 4.0f;
		std::shared_ptr<SlimeEntity> slime = std::make_shared<SlimeEntity>();
		slime->SetSlimeSize(this->size / 2);

		Vec3 newPos = { position.x + xOffset, position.y + 0.5, position.z + zOffset };
		slime->Teleport(newPos, { rand.NextFloat() * 360.0f, 0.0f });
		if (this->world)
			world->entityManager.AddEntity(slime);
	}
}

void SlimeEntity::OnCollideWithPlayer(PlayerEntity& _player) {
	if (this->size <= 1)
		return;

	const double playerPosY = _player.position.y + 1.62;

	Vec3 eyeFrom = { position.x, position.y + GetEyeHeight(), position.z };
	Vec3 eyeTo = { _player.position.x, playerPosY + 0.12, _player.position.z };
	if (!HasLineOfSight(eyeFrom, eyeTo))
		return;

	double dx = position.x - _player.position.x;
	double dy = position.y - playerPosY;
	double dz = position.z - _player.position.z;
	float distance = float(std::sqrt(dx * dx + dy * dy + dz * dz));

	if (distance < 0.6 * this->size)
		_player.AttackEntityFrom(this, this->size);
}

void SlimeEntity::Tick() {
	HostileEntity::Tick();

	if (this->isDead && this->deathTime >= 20 && this->size > 1)
		SplitSlime();
}

void SlimeEntity::OnDeath(Entity* /*_killer*/) {
	if (this->size == 1)
		DropItemAtEntity(Items::SLIME, 1);
}

void SlimeEntity::EncodeMetadata(std::vector<PacketData::EntityMetadata::DataEntry>& _metadata) {
	Entity::EncodeMetadata(_metadata);

	_metadata.push_back(
	    { .type = PacketData::EntityMetadata::BYTE, .index = 16, .value = static_cast<int8_t>(size) });
}

void SlimeEntity::UpdateAIState() {
	TryDespawn();

	auto nearestPlayer = this->entityManager->GetClosestPlayerWithin(this->position, 8);
	if (nearestPlayer)
		this->FaceEntity(*dynamic_cast<MobileEntity*>(nearestPlayer.get()), 10.0f, 20.0f);

	if (this->onGround && this->slimeJumpDelay-- <= 0) {
		this->slimeJumpDelay = rand.NextInt(20) + 10;
		if (nearestPlayer)
			this->slimeJumpDelay /= 3;

		this->jumping = true;

		this->input.x = 1.0f - rand.NextFloat() * 2.0f;
		this->input.y = this->size;
	} else {
		this->jumping = false;
		if (this->onGround)
			this->input = {};
	}
}

bool SlimeEntity::DecodeMetadata(const std::vector<PacketData::EntityMetadata::DataEntry>& _metadata) {
	if (!Entity::DecodeMetadata(_metadata)) {
		return false;
	}

	bool found = false;
	if (auto* raw = FindMetadata<int8_t>(_metadata, PacketData::EntityMetadata::BYTE, 16)) {
		size = *raw;
		found = true;
	}
	return found;
}