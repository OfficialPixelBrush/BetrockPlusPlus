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
std::string CommandSpawn::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                  std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	Int32_3 ipos = _world.GetSpawnPoint(false);
	ipos.y = _world.GetHeightValue(
	    ipos.x,
	    ipos.z); // So we don't clip in the ground since get spawn point gives the raw data which defaults to y=64

	SendTeleport(_session,
	             Vec3{ double(ipos.x) + 0.5, double(ipos.y) + PLAYER_EYE_HEIGHT + 0.0625, double(ipos.z) + 0.5 });
	return "";
}