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
std::string CommandSeed::Execute(std::vector<std::string>& parameters, PlayerSession& session, WorldManager& world,
                                 std::function<void(PlayerSession&)> transferDimension) {
	Packet::ChatMessage reply;
	reply.message = "§e" + std::to_string(world.seed);
	reply.Serialize(session.stream);
	return "";
}