/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "networking/packets.h"
#include "server.h"

// Transfers the player to the opposite dimension.
// Usage:
//   /dim
std::string CommandDimension::Execute([[maybe_unused]] std::vector<std::string>& parameters, PlayerSession& session,
                                      [[maybe_unused]] WorldManager& world,
                                      std::function<void(PlayerSession&)> transferDimension,
                                      [[maybe_unused]] Server& server) {
	Packet::ChatMessage reply;
	reply.m_message = session.m_dimension == 0 ? "§7Transferring to the Nether..." : "§7Transferring to the Overworld...";
	reply.Serialize(session.m_stream);

	Dimension newDim = session.m_dimension == -1 ? Dimension::Overworld : Dimension::Nether;

	server.sendPlayerToDimension(newDim, session);

	return "";
}
