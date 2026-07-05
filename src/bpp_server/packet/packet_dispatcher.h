/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include <cstdint>

enum class PacketId : uint8_t;
struct PlayerSession;
struct WorldManager;
struct Server;

// Dispatches a single already-identified incoming packet to its handler.
namespace PacketDispatcher {
	bool dispatch(PacketId packetId, PlayerSession& session, WorldManager& sessionWorld, Server& server);
}
