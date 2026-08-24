/*
 * Copyright (c) 2025-2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "entities/entity_skeleton.h"
#include "networking/packets.h"
#include <memory>
#include <utility>

namespace {

std::string SummonEntity(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto entity = std::make_shared<SkeletonEntity>();

	Vec3 spawnPos = ctx.session->position.pos +
	                Vec3(ctx.world->rand.NextFloat() * 4 + 0.5, 0, ctx.world->rand.NextFloat() * 4 + 0.5);
	spawnPos.y = ctx.world->GetHeightValue(spawnPos.x, spawnPos.z) + 0.1;
	entity->Teleport(spawnPos);

	ctx.world->entityManager.AddEntity(std::move(entity));
	SendChat(*ctx.session, "§eSpawned entity!");
	return "";
}

} // namespace

void RegisterSummon(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(
	    strategos::Node::literal("summon").describe("Summons a smart entity").op().executes(SummonEntity));
}
