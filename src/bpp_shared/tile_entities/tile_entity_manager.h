/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "tile_entity.h"
#include <memory>
#include <vector>

// Simple wrapper so we don't have to manually add
class WorldManager;
struct TileEntityManager {
	std::vector<std::weak_ptr<TileEntity>> tickableTileEntities;

	// This flag is set while the manager ticks tile entities
	// Tile entity additions are deferred until all tile entities are done ticking
	bool scanning = false;
	std::vector<std::shared_ptr<TileEntity>> pendingAdd;

	// Tile entities that were deferred, but whose block was gone by the end of the scan.
	// They are still ticked even if they aren't in the chunk
	std::vector<std::shared_ptr<TileEntity>> orphans;

	// Initialize a tile entity into the world
	void InitializeTileEntity(const std::shared_ptr<TileEntity>& _tileEntity) {
		if (_tileEntity->canTick) {
			tickableTileEntities.push_back(_tileEntity);
		}
	}

	void TickTileEntities(WorldManager& _world);
};