/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "version.h"

// Shows the current Server version
// Usage:
//   /version
std::string CommandVersion::Execute(std::vector<std::string>& parameters, PlayerSession& session, WorldManager& world,
                                    std::function<void(PlayerSession&)> transferDimension, Server& server) {
	Packet::ChatMessage pkt;
	pkt.message = "§eCurrent " + std::string(PROJECT_NAME) + " version is " + std::string(PROJECT_VERSION_FULL_STRING);
	pkt.Serialize(session.stream);
	return "";
}