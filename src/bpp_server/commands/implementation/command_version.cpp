/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#include "../command.h"
#include "version.h"

// Shows the current Server version
// Usage:
//   /version
std::wstring CommandVersion::Execute(std::vector<std::wstring>& parameters, PlayerSession& session, WorldManager& world, std::function<void(PlayerSession&)> transferDimension) {
	Packet::ChatMessage pkt;
	pkt.message = L"\u00a77Current " + std::wstring(PROJECT_NAME) + L" version is " + std::wstring(PROJECT_VERSION_FULL_STRING);
	pkt.Serialize(session.stream);
	return L"";
}