/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../../server.h"
#include "../command.h"
#include "../command_manager.h"

// List all currently online players
// Usage:
//   /list
std::string CommandList::Execute([[maybe_unused]] std::vector<std::string>& parameters,
                                 [[maybe_unused]] PlayerSession& session, [[maybe_unused]] WorldManager& world,
                                 [[maybe_unused]] std::function<void(PlayerSession&)> transferDimension,
                                 Server& server) {
	const auto& players = server.GetPlayers();
	Packet::ChatMessage pkt;
	pkt.m_message = std::format("§7-- {} Player(s) --", players.size());
	pkt.Serialize(session.m_stream);
	pkt.m_message = "§7";
	for (size_t i = 0; i < players.size(); i++) {
		auto& p = players[i];
		if (pkt.m_message.size() + p->m_username.size() > 64) {
			pkt.Serialize(session.m_stream);
			pkt.m_message = "§7";
		}
		pkt.m_message += p->m_username + ((i < (players.size() - 1)) ? ", " : "");
	}
	pkt.Serialize(session.m_stream);
	return "";
}