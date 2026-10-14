/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "networking/packets.h"

namespace {

std::string TeleportToSpawn(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	if (!ctx.session || !ctx.session->entity) return ERROR_PLAYER_REQUIRED;
	if (!ctx.world) return ERROR_REASON_ERROR;

	WorldManager* overworld = ctx.server->GetWorldForDimension(Dimension::Overworld);
	Int32_3 ipos = overworld->GetSpawnPoint(false);
	ipos.y = overworld->GetHeightValue(ipos.x, ipos.z);
	Vec3 dest{ double(ipos.x) + 0.5, double(ipos.y) + 0.01, double(ipos.z) + 0.5 };
	if (ctx.world->thisDimension != Dimension::Overworld) {
		ctx.server->SendPlayerToDimension(Dimension::Overworld, *ctx.session, dest);
		return "";
	}
	SendTeleport(*ctx.session, dest);
	return "";
}

} // namespace

void RegisterSpawn(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("spawn").describe("Teleport to spawn").executes(TeleportToSpawn));
}
