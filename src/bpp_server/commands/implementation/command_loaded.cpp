/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"

// Shows the number of loaded chunks
// Usage:
//   /loaded
std::string CommandLoaded::Execute([[maybe_unused]] std::vector<std::string>& _parameters, PlayerSession& _session,
                                   [[maybe_unused]] WorldManager& _world,
                                   [[maybe_unused]] std::function<void(PlayerSession&)> _transferDimension,
                                   Server& _server) {
	Packet::ChatMessage reply;
	reply.message = "§e" + std::to_string(_world.chunks.size());
	reply.Serialize(_session.stream);
	return "";
}