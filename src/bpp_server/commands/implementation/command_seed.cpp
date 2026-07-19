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
                                 std::function<void(PlayerSession&)> transferDimension, Server& server) {
	Packet::ChatMessage reply;
	reply.m_message = "§e" + std::to_string(world.m_seed);
	reply.Serialize(session.m_stream);
	return "";
}