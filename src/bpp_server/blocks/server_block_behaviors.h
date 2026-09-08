/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "../packet/packet_utils.h"
#include "../player_conn/player_session.h"
#include "enums/blocks.h"
#include "runtime.h"
#include "world/world.h"
#include <numeric_structs.h>
#include <vector>

// Behavioral overrides for the base block behaviors.
// If its nullptr here, use the base block overrides. Else, use what is here.
namespace ServerBlock {
struct BlockBehavior {
	bool (*onBlockActivated)(WorldManager& _world, Int3 _position, PlayerSession& _session,
	                         Runtime& _gameRuntime) = nullptr;
};
extern BlockBehavior blockBehaviors[BLOCK_MAX];

void Initialize();

// Attempts to spawn hostile mobs near a sleeping player's bed and, if any of
// them manage to path to it, wakes the player up
void TrySpawnNightmare(WorldManager& _world, PlayerSession& _session);
} // namespace ServerBlock