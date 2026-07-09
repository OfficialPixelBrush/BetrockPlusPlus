/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "networking/packets.h"

// Transfers the player to the opposite dimension.
// Usage:
//   /dim
std::string CommandDimension::Execute([[maybe_unused]] std::vector<std::string>& parameters, PlayerSession& session, [[maybe_unused]] WorldManager& world,
                                      std::function<void(PlayerSession&)> transferDimension) {
	transferDimension(session);

	Packet::ChatMessage reply;
	reply.message = session.dimension == -1 ? "§eTransferring to the Nether..." : "§7Transferring to the Overworld...";
	reply.Serialize(session.stream);

	return "";
}
