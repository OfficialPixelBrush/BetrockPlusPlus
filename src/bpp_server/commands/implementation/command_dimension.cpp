/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#include "../command.h"
#include "networking/packets.h"

// Transfers the player to the opposite dimension.
// Usage:
//   /dim
std::string CommandDimension::Execute(std::vector<std::string>& parameters, PlayerSession& session,
                                       WorldManager& world, std::function<void(PlayerSession&)> transferDimension) {
	transferDimension(session);

	Packet::ChatMessage reply;
	reply.message = session.dimension == -1 ? "\u00a77Transferring to the Nether..."
	                                        : "\u00a77Transferring to the Overworld...";
	reply.Serialize(session.stream);

	return "";
}
