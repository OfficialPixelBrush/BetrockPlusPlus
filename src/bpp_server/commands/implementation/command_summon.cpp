/*
 * Copyright (c) 2025-2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "../command.h"
#include "./entities/entity_dummy_player.h"
#include "networking/packets.h"
#include "server.h"
#include <memory>
#include <utility>

std::string CommandSummon::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                   std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	// Make a dummy player
	auto entity = std::make_shared<DummyMPPlayer>(_server.gameRuntime, _server);

	Vec3 spawnPos = _session.position.pos +
	                Vec3(_world.rand.NextFloat() * 4 + 0.5, 0, _world.rand.NextFloat() * 4 + 0.5);
	spawnPos.y = _world.GetHeightValue(spawnPos.x, spawnPos.z) + 0.1;
	entity->Teleport(spawnPos);

	entity->dummySession.dimension = entity->dim;
	entity->dummySession.entityTracker = _server.GetEntityTrackerForDimension(entity->dim);
	entity->dummySession.username = " ";

	_world.entityManager.AddEntity(std::move(entity));

	Packet::ChatMessage pkt;
	pkt.message = "§eSpawned player entity!";
	pkt.Serialize(_session.stream);
	return "";
}