/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../../server.h"

// List all currently online players
// Usage:
//   /list
std::string CommandList::Execute([[maybe_unused]] std::vector<std::string>& parameters, [[maybe_unused]] PlayerSession& session, [[maybe_unused]] WorldManager& world,
                                 [[maybe_unused]] std::function<void(PlayerSession&)> transferDimension, Server& server) {
	const auto& players = server.GetPlayers();
	Packet::ChatMessage pkt;
	pkt.message = std::format("§7-- {} Player(s) --", players.size());
	pkt.Serialize(session.stream);
	pkt.message = "§7";
	for (size_t i = 0; i < players.size(); i++) {
		auto& p = players[i];
		if (pkt.message.size()+p->username.size() > 64) {
			pkt.Serialize(session.stream);
			pkt.message = "§7";
		}
		pkt.message += p->username + ((i < (players.size() - 1)) ? ", " : "");
	}
	pkt.Serialize(session.stream);
	return "";
}