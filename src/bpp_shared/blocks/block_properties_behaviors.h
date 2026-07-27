/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include "blocks/block_behaviors.h"
#include "blocks/block_properties.h"

namespace Blocks {
// Call once at startup before anything reads from the tables
void RegisterAll();
}; // namespace Blocks