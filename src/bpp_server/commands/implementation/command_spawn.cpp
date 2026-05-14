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
std::wstring CommandSpawn::Execute(std::vector<std::wstring>& parameters, PlayerSession& session, WorldManager& world) {
    Int32_3 ipos = world.getSpawnPoint(false);
    SendTeleport(
        session, 
        Double3{
            double(ipos.x) + 0.5,
            double(ipos.y) + PLAYER_EYE_HEIGHT + 0.5,
            double(ipos.z) + 0.5
        }
    );
    return L"";
}