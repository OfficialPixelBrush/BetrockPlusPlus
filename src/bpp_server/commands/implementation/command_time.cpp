/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#include "../command.h"

// Gets or sets the current world time
// Usage:
//   /time
//   /time <new_time>
std::wstring CommandTime::Execute(std::vector<std::wstring>& parameters, PlayerSession& session, WorldManager& world, std::function<void(PlayerSession&)> transferDimension) {
	// Set the time
	if (parameters.size() > 1) {
		world.elapsed_ticks = std::stol(parameters[1]);

		Packet::ChatMessage reply;
		reply.message = L"\u00a77Set time to " + parameters[1];
		reply.Serialize(session.stream);
		return L"";
	}
	// Get the time
	if (parameters.size() == 1) {
		Packet::ChatMessage reply;
		reply.message = L"\u00a77Current Time is " + std::to_wstring(world.elapsed_ticks);
		reply.Serialize(session.stream);
		return L"";
	}
	return ERROR_REASON_SYNTAX;
}