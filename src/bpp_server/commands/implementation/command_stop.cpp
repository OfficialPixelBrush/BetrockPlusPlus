/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "server.h"
#include <cstdint>
#include <string>

// Fills an area with the desired block
// Usage:
//   /stop [time]/cancel
std::string CommandStop::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                 std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	if (_parameters.size() < 2) {
		_server.SendGlobalChatMessage(std::format("§eStopping..."));
		shutdownRequested.store(true);
		return "";
	}

	// Parse parameters
	size_t paramOffset = 1;
	if (_parameters[paramOffset] == "cancel") {
		_server.ResetTimeout();
		_server.SendGlobalChatMessage(std::format("§eCancelled stop!"));
		return "";
	}

	float timeout = 0.0f;
	try {
		timeout = std::stof(_parameters[paramOffset]);
	} catch (...) {
		return ERROR_REASON_PARAMETERS;
	}
	static constexpr float MAX_TIMEOUT = UINT16_MAX / _server.TICKS_PER_SECOND;
	if (timeout > MAX_TIMEOUT)
		return std::format("Exceeds max timeout! ({} seconds)", MAX_TIMEOUT);

	// Inform all players
	_server.SendGlobalChatMessage(std::format("§eStopping in {:.1f} seconds...", timeout));
	_server.StopTimeout(timeout);
	return "";
}