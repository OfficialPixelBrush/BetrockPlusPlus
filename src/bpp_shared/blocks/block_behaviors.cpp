/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "blocks/block_behaviors.h"
#include "block_behaviors/internal.h"

// The actual per-block registration logic lives in block_behaviors/*.cpp, one
// file per category (fluids, rails, redstone, plants, doors, beds, chests,
// falling blocks, portals, misc, and block drops). This file just wires them
// together, which keeps each category independently readable and lets the
// build compile them in parallel.

namespace Blocks {

BlockBehavior blockBehaviors[BLOCK_MAX] = {};

void RegisterBlockBehaviors() {
	RegisterFluidBehaviors();
	RegisterRailBehaviors();
	RegisterRedstoneBehaviors();
	RegisterPlantBehaviors();
	RegisterDoorBehaviors();
	RegisterBedBehaviors();
	RegisterChestBehaviors();
	RegisterFallingBlockBehaviors();
	RegisterPortalBehaviors();
	RegisterMiscBehaviors();
	RegisterBlockDrops();
}

}; // namespace Blocks
