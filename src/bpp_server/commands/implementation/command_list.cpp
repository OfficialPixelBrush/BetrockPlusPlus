/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"

// List all currently online players
// Usage:
//   /list
std::string CommandList::Execute(std::vector<std::string>& parameters, PlayerSession& session, WorldManager& world,
                                 std::function<void(PlayerSession&)> transferDimension) {
	Packet::ChatMessage pkt;
	pkt.message = "§7-- All players --";
	pkt.Serialize(session.stream);
	pkt.message = "§7";
	size_t i = 0;
	return "";
}