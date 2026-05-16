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
std::wstring CommandDimension::Execute(std::vector<std::wstring>& parameters, PlayerSession& session, WorldManager& world, std::function<void(PlayerSession&)> transferDimension) {
    transferDimension(session);

    Packet::ChatMessage reply;
    reply.message = session.dimension == -1
        ? L"\u00a77Transferring to the Nether..."
        : L"\u00a77Transferring to the Overworld...";
    reply.Serialize(session.stream);

    return L"";
}
