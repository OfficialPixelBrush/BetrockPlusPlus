/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "numeric_structs.h"
#include "world/world.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

class Server;

// Sends accumulated per-tick block changes out to whichever player sessions care about them
namespace ChunkBroadcaster {
void broadcastBlockChanges(Server& server, std::unordered_map<Int32_2, std::vector<PendingBlock>>& changes,
                           int8_t dimension, WorldManager& dimWorld);
} // namespace ChunkBroadcaster
