/*
 * Copyright (c) 2025-2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "../command.h"
#include "entities/entity_mobile.h"
#include "networking/packets.h"
#include <memory>
#include <utility>

std::string CommandSummon::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                   std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	auto entity = std::make_shared<MobileEntity>();
	Vec3 spawnPos = _session.position.pos;
	entity->Teleport(spawnPos);
	_world.entityManager.AddEntity(std::move(entity));

	Packet::ChatMessage pkt;
	pkt.message = "§eSpawned smart entity at " + spawnPos.Str();
	pkt.Serialize(_session.stream);
	return "";
}