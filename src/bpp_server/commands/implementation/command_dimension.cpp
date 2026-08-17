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
std::string CommandDimension::Execute([[maybe_unused]] std::vector<std::string>& _parameters, PlayerSession& _session,
                                      [[maybe_unused]] WorldManager& _world,
                                      std::function<void(PlayerSession&)> _transferDimension,
                                      [[maybe_unused]] Server& _server) {
	Packet::ChatMessage reply;
	reply.message = _session.dimension == Dimension::Overworld ? "§7Transferring to the Nether..." : "§7Transferring to the Overworld...";
	reply.Serialize(_session.stream);

	Dimension newDim = _session.dimension == Dimension::Nether ? Dimension::Overworld : Dimension::Nether;

	_server.SendPlayerToDimension(newDim, _session);

	return "";
}
