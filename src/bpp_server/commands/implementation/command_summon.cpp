/*
 * Copyright (c) 2025-2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "entities/entity_minecart.h"
#include "networking/packets.h"
#include <memory>
#include <utility>

namespace {

EntityType GetEntityFromString(std::string name) {
	if (name == "creeper") {
		return EntityType::CREEPER;
	}
	if (name == "boat") {
		return EntityType::BOAT;
	}
	if (name == "minecart") {
		return Entity::MINECART;
	}
}

std::string SummonEntity(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);

	auto entity = std::make_shared<MinecartEntity>();

	Vec3 spawnPos = ctx.session->position.pos;
	spawnPos.y += 1.0f; // Spawn above the player
	entity->Teleport(spawnPos);

	ctx.world->entityManager.AddEntity(entity);

	SendChat(*ctx.session, "§eSpawned entity!");
	return "";
}

} // namespace

void RegisterSummon(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(
	    strategos::Node::literal("summon").describe("Summons a smart entity").op().executes(SummonEntity));
}
