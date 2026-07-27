/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "tile_entity_manager.h"
#include "world/world.h"

void TileEntityManager::TickTileEntities(WorldManager& _world) {
	std::erase_if(tickableTileEntities, [&](const std::weak_ptr<TileEntity>& _wp) {
		auto te = _wp.lock();
		if (!te)
			return true;
		te->Tick(_world);
		return false;
	});
}