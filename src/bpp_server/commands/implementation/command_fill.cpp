/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "blocks.h"
#include "server.h"

// Fills an area with the desired block
// Usage:
//   /fill <block:meta> <x0> <y0> <z0> <x1> <y1> <z1> [tick]
std::string CommandFill::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                 std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	Int3 pos0, pos1;
	// TODO: Implement!
	return ERROR_REASON_SYNTAX;
}