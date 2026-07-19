/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "../command.h"
#include "networking/packets.h"

// Teleport to spawn
// Usage:
//   /spawn
std::string CommandSpawn::Execute(std::vector<std::string>& parameters, PlayerSession& session, WorldManager& world,
                                  std::function<void(PlayerSession&)> transferDimension, Server& server) {
	Int32_3 ipos = world.getSpawnPoint(false);
	ipos.m_y = world.getHeightValue(
	    ipos.m_x,
	    ipos.m_z); // So we don't clip in the ground since get spawn point gives the raw data which defaults to y=64

	SendTeleport(session,
	             Vec3{ double(ipos.m_x) + 0.5, double(ipos.m_y) + PLAYER_EYE_HEIGHT + 0.0625, double(ipos.m_z) + 0.5 });
	return "";
}