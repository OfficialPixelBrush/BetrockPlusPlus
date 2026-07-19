/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"

// Gets or sets the current world time
// Usage:
//   /time
//   /time <new_time>
std::string CommandTime::Execute(std::vector<std::string>& parameters, PlayerSession& session, WorldManager& world,
                                 std::function<void(PlayerSession&)> transferDimension, Server& server) {
	// Set the time
	if (parameters.size() > 2) {
		if (parameters[1] == "set") {
			world.m_elapsed_ticks = std::stol(parameters[2]);
		} else if (parameters[1] == "add") {
			world.m_elapsed_ticks += std::stol(parameters[2]);
		} else {
			return "Invalid argument " + parameters[1];
		}
		Packet::ChatMessage reply;
		reply.m_message = "§eSet time to " + std::to_string(world.m_elapsed_ticks);
		reply.Serialize(session.m_stream);
		return "";
	}

	// Get the time
	if (parameters.size() == 1) {
		Packet::ChatMessage reply;
		reply.m_message = "§eCurrent Time is " + std::to_string(world.m_elapsed_ticks);
		reply.Serialize(session.m_stream);
		return "";
	}
	return ERROR_REASON_SYNTAX;
}