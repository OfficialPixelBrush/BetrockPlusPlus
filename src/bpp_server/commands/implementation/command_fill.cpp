/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "blocks.h"
#include "server.h"
#include "logger.h"
#include <string>

// Fills an area with the desired block
// Usage:
//   /fill <block:meta> <x0> <y0> <z0> <x1> <y1> <z1>
std::string CommandFill::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                 std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	// Parse parameters
	size_t paramOffset = 1;
	ItemStack item = ParseItemStack(_parameters, paramOffset, false);

	if (item.id >= BLOCK_MAX || item.id < 0)
		return "Invalid Block Id!";

	Int3 pos0, pos1;

	try {
		pos0.x = static_cast<int32_t>(std::stoi(_parameters[paramOffset++]));
		pos0.y = static_cast<int32_t>(std::stoi(_parameters[paramOffset++]));
		pos0.z = static_cast<int32_t>(std::stoi(_parameters[paramOffset++]));
		pos1.x = static_cast<int32_t>(std::stoi(_parameters[paramOffset++]));
		pos1.y = static_cast<int32_t>(std::stoi(_parameters[paramOffset++]));
		pos1.z = static_cast<int32_t>(std::stoi(_parameters[paramOffset++]));
	} catch (...) {
		return ERROR_REASON_PARAMETERS;
	}
	if (pos0.y >= CHUNK_HEIGHT || pos0.y < 0 || pos1.y >= CHUNK_HEIGHT || pos1.y < 0)
		return ERROR_REASON_PARAMETERS;

	// Calculate volume
	int64_t width  = std::abs(pos1.x - pos0.x) + 1;
	int64_t height = std::abs(pos1.y - pos0.y) + 1;
	int64_t depth  = std::abs(pos1.z - pos0.z) + 1;

	int64_t volume = width * height * depth;

	Packet::ChatMessage pkt;
	pkt.message = std::format("Attemping to fill {} block(s)...", volume);
	pkt.Serialize(_session.stream);

	// Perform fill
	const Int3 start = pos0;
	
	auto fillStart = std::chrono::steady_clock::now();
	for (pos0.x = start.x; pos0.x <= pos1.x; ++pos0.x) {
		for (pos0.y = start.y; pos0.y <= pos1.y; ++pos0.y) {
			for (pos0.z = start.z; pos0.z <= pos1.z; ++pos0.z) {
				// TODO: Probably not the ideal approach, but this works
				_world.SetBlock(pos0,
								static_cast<BlockType>(item.id.value),
								static_cast<uint8_t>(item.data));
			}
		}
	}
	float fillSeconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - fillStart).count();

	// Send message back to player
	Packet::ChatMessage pktEnd;
	pktEnd.message = std::format("Filled {} block(s) in {:.2f} seconds!", volume, fillSeconds);
	pktEnd.Serialize(_session.stream);
	return "";
}