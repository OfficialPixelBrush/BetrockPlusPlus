/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#include "../command.h"

// Shows the number of loaded chunks
// Usage:
//   /loaded
std::wstring CommandLoaded::Execute(std::vector<std::wstring>& parameters, PlayerSession& session, WorldManager& world, std::function<void(PlayerSession&)> transferDimension) {
	Packet::ChatMessage reply;
	reply.message = L"\u00a77" + std::to_wstring(world.chunks.size());
	reply.Serialize(session.stream);
	return L"";
}