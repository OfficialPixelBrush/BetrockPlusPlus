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
	Int32_3 ipos = ctx.world->GetSpawnPoint(false);
	ipos.y = ctx.world->GetHeightValue(ipos.x, ipos.z);

	SendTeleport(*ctx.session, Vec3{ double(ipos.x) + 0.5, double(ipos.y) + 0.01, double(ipos.z) + 0.5 });
	return "";
}

} // namespace

void RegisterSpawn(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("spawn").describe("Teleport to spawn").executes(TeleportToSpawn));
}
