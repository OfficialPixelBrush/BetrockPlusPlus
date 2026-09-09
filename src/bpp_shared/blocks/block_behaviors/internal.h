/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "blocks/block_behaviors.h"

class WorldManager;

namespace Blocks {

void RegisterFluidBehaviors();
void RegisterRailBehaviors();
void RegisterRedstoneBehaviors();
void RegisterPlantBehaviors();
void RegisterDoorBehaviors();
void RegisterBedBehaviors();
void RegisterChestBehaviors();
void RegisterFallingBlockBehaviors();
void RegisterPortalBehaviors();
void RegisterMiscBehaviors();
void RegisterBlockDrops();

// Defined in common.cpp
bool IsReplaceable(WorldManager& _world, Int3 _pos);
bool IsSupported(WorldManager& _world, Int3 _pos, Direction::Value _dir);

}; // namespace Blocks
