/*
 * Copyright (c) 2025-2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "../command.h"
#include "entities/entity_pig.h"
#include "networking/packets.h"
#include <memory>
#include <utility>

std::string CommandSummon::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                   std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	for (int i = 0; i < 100; i++) {
		auto entity = std::make_shared<PigEntity>();
		Vec3 spawnPos = _session.position.pos + Vec3(_world.rand.NextFloat() * 16, 0, _world.rand.NextFloat() * 16);
		spawnPos.y = _world.GetHeightValue(spawnPos.x, spawnPos.z);
		entity->Teleport(spawnPos);
		_world.entityManager.AddEntity(std::move(entity));
	}

	Packet::ChatMessage pkt;
	pkt.message = "§eSpawned smart entities";
	pkt.Serialize(_session.stream);
	return "";
}