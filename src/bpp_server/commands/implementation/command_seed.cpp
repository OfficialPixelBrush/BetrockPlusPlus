/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#include "../command.h"

// Get the world seed
// Usage:
//   /seed
std::wstring CommandSeed::Execute(std::vector<std::wstring>& parameters, PlayerSession& session, WorldManager& world, std::function<void(PlayerSession&)> transferDimension) {
	Packet::ChatMessage reply;
	reply.message = L"\u00a77" + std::to_wstring(world.seed);
	reply.Serialize(session.stream);
	return L"";
}