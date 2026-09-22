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
	const WorldManager& overworld = ctx.server->gameRuntime.world;
	const WorldManager& nether = ctx.server->gameRuntime.worldHell;
	const size_t chunksOverworld = overworld.chunks.size();
	const size_t chunksNether = nether.chunks.size();
	const double chunksOverworldMb = static_cast<double>(chunksOverworld * sizeof(Chunk)) / 1024.0 / 1024.0;
	const double chunksNetherMb = static_cast<double>(chunksNether * sizeof(Chunk)) / 1024.0 / 1024.0;
	const double totalUsageMb = GetMemoryUsage(MemoryUnit::Megabyte);
	const std::array<std::string, 5> lines = {
		std::format("§7Total Usage: {:.2f} MB", totalUsageMb),
		std::format("§7World Chunks: {} (~{:.1f} MB, {:.1f}%)", chunksOverworld, chunksOverworldMb, (chunksOverworldMb/totalUsageMb)*100.0),
		std::format("§7Hell Chunks: {} (~{:.1f} MB, {:.1f}%)", chunksNether, chunksNetherMb, (chunksNetherMb/totalUsageMb)*100.0),
		std::format("§7{} Players, {} Entities, {} Hell Entities", players.size(),
		            ctx.server->overworldEntityTracker.trackedEntities.size(), ctx.server->hellEntityTracker.trackedEntities.size()),
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
