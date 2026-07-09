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
	if (session.players) {
		for (const auto& sessionPtr : *session.players) {
			if (!sessionPtr)
				continue;
			PlayerSession& localSession = *sessionPtr;
			pkt.message += localSession.username;
			if (i < session.players->size() - 1) {
				pkt.message += ", ";
			}
			if (pkt.message.size() > MAX_CHAT_LINE_SIZE || i == session.players->size() - 1) {
				pkt.Serialize(session.stream);
				pkt.message = "§7";
			}
			i++;
		}
	}
	return "";
}