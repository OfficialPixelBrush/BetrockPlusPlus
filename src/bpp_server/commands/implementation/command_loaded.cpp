/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"

// Shows the number of loaded chunks
// Usage:
//   /loaded
std::string CommandLoaded::Execute([[maybe_unused]] std::vector<std::string>& parameters, PlayerSession& session,
                                   [[maybe_unused]] WorldManager& world,
                                   [[maybe_unused]] std::function<void(PlayerSession&)> transferDimension,
                                   Server& server) {
	Packet::ChatMessage reply;
	reply.message = "§e" + std::to_string(world.chunks.size());
	reply.Serialize(session.stream);
	return "";
}