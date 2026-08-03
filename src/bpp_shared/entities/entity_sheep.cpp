/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_sheep.h"

void SheepEntity::OnDeath() {
	DropItemAtEntity(BLOCK_WOOL, 1, this->color);
}
