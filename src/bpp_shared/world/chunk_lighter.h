/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include <vector>
#include "light_packet.h"
#include "chunk.h"

// Chunk local lighter that is safe to run on independent threads.
// This lighter can send "packets" to the global lighter to send lighting updates to OTHER chunks.

// This is just a namespace since these lighters don't really have a "state"
namespace ChunkLighter {
	std::vector<LightPacket> propagateSkyLights(Chunk& chunk, LightPacket light_packet) {
		
	}

	std::vector<LightPacket> propagateBlockLights(Chunk& chunk, LightPacket light_packet) {

	}
}