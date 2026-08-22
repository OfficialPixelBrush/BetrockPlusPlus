/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../../../bpp_shared/helpers/hardware.h"
#include "../command.h"
#include "chunk.h"
#include "server.h"
#include <format>
#include <string>

// Shows usage statistics
// Usage:
//   /stats
std::string CommandStats::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                  std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	if (!HasPermissions(_session))
		return ERROR_PERMISSIONS;
	const auto& players = _server.GetPlayers();
	double chunksMb = static_cast<double>(_world.chunks.size() * sizeof(Chunk)) / 1024.0 / 1024.0;
	std::array<std::string, 4> lines = {
		std::format("§7Mem: {:.2f} MB", GetMemoryUsage(MemoryUnit::Megabyte)),
		std::format("§7{} Chunks (Aprox. {:.2f} MB)", _world.chunks.size(), chunksMb),
		std::format("§7{} Players, {} Entities", players.size(), _server.overworldEntityTracker.trackedEntities.size()),
		std::format("§7Avg. MSPT: {:.2f} ms", _server.averageTickMs),
	};
	for (int i = 0; i < 4; i++) {
		Packet::ChatMessage pkt;
		pkt.message = lines[i];
		pkt.Serialize(_session.stream);
	}
	return "";
}