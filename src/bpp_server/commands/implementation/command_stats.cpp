/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "server.h"

// Shows usage statistics
// Usage:
//   /stats
std::string CommandStats::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                  std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	Packet::ChatMessage tickMs;
	tickMs.message = "§eAvg. MSPT: " + std::to_string(_server.averageTickMs) + " ms";
	tickMs.Serialize(_session.stream);
	return "";
}