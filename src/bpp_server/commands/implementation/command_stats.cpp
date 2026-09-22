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

std::string GenerateUsageBar(double _total, double _active, double _overworld, double _nether) {
    std::string bar = "§f[";
    if (_total <= 0.0) {
        return bar + std::string(30, ' ') + "§f]";
    }
    // Clamp values to valid ranges.
    double total = _total;
    double active = std::clamp(_active, 0.0, total);
    double overworld = std::clamp(_overworld, 0.0, active);
    double nether = std::clamp(_nether, 0.0, active - overworld);
    // Calculate cumulative boundaries.
    double overworldEnd = overworld / total;
    double netherEnd = (overworld + nether) / total;
    double activeEnd = active / total;

    for (int i = 0; i < 30; i++) {
        double position = (i + 0.5) / 30.0;
        if (position < overworldEnd) {
            bar += "§a#"; // Overworld
        }
        else if (position < netherEnd) {
            bar += "§c#"; // Nether
        }
        else if (position < activeEnd) {
            bar += "§e#"; // Other active usage
        }
        else {
            bar += "§0#"; // Inactive
        }
    }
    bar += "§f]";
    return bar;
}

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
	const double activeUsageMb = GetActiveMemoryUsage(MemoryUnit::Megabyte);
	const std::array<std::string, 5> lines = {
		std::format("§7Alloc: {}", GenerateUsageBar(totalUsageMb, activeUsageMb, chunksOverworldMb, chunksNetherMb)),
		std::format("§7Total: {:.2f} MB, Active: {:.2f} MB", totalUsageMb, activeUsageMb),
		std::format("§7World: {} (~{:.1f} MB), Hell: {} (~{:.1f} MB)", chunksOverworld, chunksOverworldMb, chunksNether, chunksNetherMb),
		std::format("§7{} Players, {} Entities, {} Hell Entities", players.size(),
		            ctx.server->overworldEntityTracker.trackedEntities.size(), ctx.server->hellEntityTracker.trackedEntities.size()),
		std::format("§7Avg. MSPT: {:.2f} ms", ctx.server->averageTickMs)
	};
	for (const auto& line : lines)
		SendChat(*ctx.session, line);
	return "";
}

} // namespace

void RegisterStats(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("stats").describe("Shows usage statistics").executes(ShowStats));
}
