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
	Vec3 spawnPos = session.m_position.m_pos;
	entity->teleport(spawnPos);
	world.m_entityManager.addEntity(std::move(entity));

	Packet::ChatMessage pkt;
	pkt.m_message = "§eSpawned smart entity at " + spawnPos.str();
	pkt.Serialize(session.m_stream);
	return "";
}