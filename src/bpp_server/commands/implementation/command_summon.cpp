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

std::string CommandSummon::Execute(std::vector<std::string>& parameters, PlayerSession& session, WorldManager& world,
                                  std::function<void(PlayerSession&)> transferDimension, Server& server) {
	auto entity = std::make_shared<MobileEntity>();
	Vec3 spawnPos = session.position.pos;
	entity->teleport(spawnPos);
	world.entityManager.addEntity(std::move(entity));

	Packet::ChatMessage pkt;
	pkt.message = "§eSpawned smart entity at " + spawnPos.str();
	pkt.Serialize(session.stream);
	return "";
}