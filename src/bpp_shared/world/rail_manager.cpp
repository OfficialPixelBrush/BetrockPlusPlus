/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "world.h"
#include "rail_manager.h"

bool RailManager::IsRailPowered(WorldManager& _world, Int3 _pos) {
	auto block = _world.GetBlockId(_pos);
	if (block == BLOCK_RAIL_POWERED)
		return _world.GetMetadata(_pos) & 0b1000;
	return false;
}