/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "tile_entity_noteblock.h"

void TileEntityNoteblock::Tick(WorldManager& /*_world*/) {
    // TODO
}

Tag TileEntityNoteblock::Serialize() {
	auto root = TileEntity::Serialize();
	root.compound["note"] = Tag{ .type = TAG_BYTE,
		                             .name = "note",
		                             .byteValue = static_cast<int8_t>(note) };
	return root;
}