/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../../../bpp_shared/helpers/hardware.h"
#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "chunk.h"
#include "server.h"
#include <array>
#include <format>

namespace {

std::string ShowStats(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	const auto& players = ctx.server->GetPlayers();
	double chunksMb = static_cast<double>(ctx.world->chunks.size() * sizeof(Chunk)) / 1024.0 / 1024.0;
	std::array<std::string, 4> lines = {
		std::format("§7Mem: {:.2f} MB", GetMemoryUsage(MemoryUnit::Megabyte)),
		std::format("§7{} Chunks (Aprox. {:.2f} MB)", ctx.world->chunks.size(), chunksMb),
		std::format("§7{} Players, {} Entities", players.size(), ctx.server->overworldEntityTracker.trackedEntities.size()),
		std::format("§7Avg. MSPT: {:.2f} ms", ctx.server->averageTickMs),
	};
	for (const auto& line : lines)
		SendChat(*ctx.session, line);
	return "";
}

} // namespace

void RegisterStats(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("stats").describe("Shows usage statistics").executes(ShowStats));
}
