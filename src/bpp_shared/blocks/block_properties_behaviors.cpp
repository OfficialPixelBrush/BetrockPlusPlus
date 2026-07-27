/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "block_properties_behaviors.h"

namespace Blocks {
void RegisterAll() {
	// Default all behavior slots to full-cube before per-block overrides
	for (int i = 0; i < 256; i++) {
		blockBehaviors[i].getSelectionBox = DefaultAabb;
		blockBehaviors[i].getRayBounds = DefaultAabb;
		blockBehaviors[i].getCollider = DefaultCollider;
	}
	RegisterBlockProperties();
	RegisterBlockBehaviors();
}
} // namespace Blocks