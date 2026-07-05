/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/
#include "../command.h"
#include "networking/packets.h"

// Teleport to spawn
// Usage:
//   /spawn
std::wstring CommandSpawn::Execute(std::vector<std::wstring>& parameters, PlayerSession& session, WorldManager& world,
                                   std::function<void(PlayerSession&)> transferDimension) {
	Int32_3 ipos = world.getSpawnPoint(false);
	world.forceGenChunkSync({ ipos.x >> 4, ipos.z >> 4 });
	ipos.y = world.getHeightValue(
	    ipos.x,
	    ipos.z); // So we don't clip in the ground since get spawn point gives the raw data which defaults to y=64

	SendTeleport(
	    session, Vec3 {
		    double(ipos.x) + 0.5, double(ipos.y) + PLAYER_EYE_HEIGHT + session.position.pos.y + 0.0625, double(ipos.z) + 0.5
	    });
	return L"";
}