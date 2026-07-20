/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include <string>

// Get the world seed
// Usage:
//   /seed
std::string CommandSeed::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                 std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	Packet::ChatMessage reply;
	reply.message = "§e" + std::to_string(_world.seed);
	reply.Serialize(_session.stream);
	return "";
}