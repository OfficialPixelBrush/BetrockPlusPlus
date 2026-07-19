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
	bool (*m_onBlockActivated)(WorldManager& world, Int3 position, PlayerSession& session, Runtime& gameRuntime) = nullptr;
};
extern BlockBehavior blockBehaviors[256];

void initialize();
} // namespace ServerBlock