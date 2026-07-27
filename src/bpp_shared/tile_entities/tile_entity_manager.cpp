/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "tile_entity_manager.h"
#include "world/world.h"

void TileEntityManager::TickTileEntities(WorldManager& _world) {
	// Snapshot for iteration so Tick() can safely mutate tickableTileEntities
	std::vector<std::weak_ptr<TileEntity>> tickableTileEntitiesCopy = tickableTileEntities;

	for (const std::weak_ptr<TileEntity>& wp : tickableTileEntitiesCopy) {
		if (auto te = wp.lock()) {
			te->Tick(_world);
		}
	}

	std::erase_if(tickableTileEntities, [](const std::weak_ptr<TileEntity>& _wp) { return _wp.expired(); });
}