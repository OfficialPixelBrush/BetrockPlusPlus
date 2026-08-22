/*
 * Copyright (c) 2025-2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "../command.h"
#include "entities/entity_creeper.h"
#include "networking/packets.h"
#include "server.h"
#include <memory>
#include <utility>

std::string CommandSummon::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                   std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	if (!HasPermissions(_session))
		return ERROR_PERMISSIONS;
	// Make a dummy player
	for (int i = 0; i < 400; i++) {
		auto entity = std::make_shared<CreeperEntity>();

		Vec3 spawnPos = _session.position.pos +
		                Vec3(_world.rand.NextFloat() * 4 + 0.5, 0, _world.rand.NextFloat() * 4 + 0.5);
		spawnPos.y = _world.GetHeightValue(spawnPos.x, spawnPos.z) + 0.1;
		entity->Teleport(spawnPos);

		_world.entityManager.AddEntity(std::move(entity));
	}
	Packet::ChatMessage pkt;
	pkt.message = "§eSpawned entity!";
	pkt.Serialize(_session.stream);
	return "";
}