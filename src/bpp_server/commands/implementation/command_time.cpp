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
std::string CommandTime::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                 std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	// Set the time
	if (_parameters.size() > 2) {
		if (_parameters[1] == "set") {
			_world.elapsedTicks = std::stol(_parameters[2]);
		} else if (_parameters[1] == "add") {
			_world.elapsedTicks += std::stol(_parameters[2]);
		} else {
			return "Invalid argument " + _parameters[1];
		}
		Packet::ChatMessage reply;
		reply.message = "§eSet time to " + std::to_string(_world.elapsedTicks);
		reply.Serialize(_session.stream);
		return "";
	}

	// Get the time
	if (_parameters.size() == 1) {
		Packet::ChatMessage reply;
		reply.message = "§eCurrent Time is " + std::to_string(_world.elapsedTicks);
		reply.Serialize(_session.stream);
		return "";
	}
	return ERROR_REASON_SYNTAX;
}