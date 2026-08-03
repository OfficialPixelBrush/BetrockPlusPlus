/*
 * Copyright (c) 2025-2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "../command.h"
#include "entities/entity_pig.h"
#include "entities/entity_chicken.h"
#include "entities/entity_sheep.h"
#include "entities/entity_cow.h"
#include "networking/packets.h"
#include <memory>
#include <utility>

std::string CommandSummon::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                   std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	for (int ti = 0; ti < 2; ti++) {
		auto entity = std::make_shared<PigEntity>();
		auto entity1 = std::make_shared<CowEntity>();
		auto entity2 = std::make_shared<SheepEntity>();
		auto entity3 = std::make_shared<ChickenEntity>();

		Vec3 spawnPos = _session.position.pos + Vec3(_world.rand.NextFloat() * 4 + 0.5, 0, _world.rand.NextFloat() * 4 + 0.5);
		spawnPos.y = _world.GetHeightValue(spawnPos.x, spawnPos.z) + 0.1;
		entity->Teleport(spawnPos);
		spawnPos = _session.position.pos + Vec3(_world.rand.NextFloat() * 4 + 0.5, 0, _world.rand.NextFloat() * 4 + 0.5);
		spawnPos.y = _world.GetHeightValue(spawnPos.x, spawnPos.z) + 0.1;
		entity1->Teleport(spawnPos);
		spawnPos = _session.position.pos + Vec3(_world.rand.NextFloat() * 4 + 0.5, 0, _world.rand.NextFloat() * 4 + 0.5);
		spawnPos.y = _world.GetHeightValue(spawnPos.x, spawnPos.z) + 0.1;
		entity2->Teleport(spawnPos);
		spawnPos = _session.position.pos + Vec3(_world.rand.NextFloat() * 4 + 0.5, 0, _world.rand.NextFloat() * 4 + 0.5);
		spawnPos.y = _world.GetHeightValue(spawnPos.x, spawnPos.z) + 0.1;
		entity3->Teleport(spawnPos);

		_world.entityManager.AddEntity(std::move(entity));
		_world.entityManager.AddEntity(std::move(entity1));
		_world.entityManager.AddEntity(std::move(entity2));
		_world.entityManager.AddEntity(std::move(entity3));
	}

	Packet::ChatMessage pkt;
	pkt.message = "§eSpawned smart entities";
	pkt.Serialize(_session.stream);
	return "";
}