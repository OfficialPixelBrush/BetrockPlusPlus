/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include "items.h"
#include "base_structs.h"

// Returns true if the destroyed item maintains its NBT data upon being dropped
bool KeepDamageOnDrop(int8_t id);

// Returns true for all blocks that do not drop anything when they're destroyed
bool NoDrop(Item item);

// Returns the items that're dropped when a block is destroyed
Item GetDrop(Item item);