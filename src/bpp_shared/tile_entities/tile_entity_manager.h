/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "tile_entity.h"
#include <algorithm>
#include <memory>
#include <vector>

// Mirrors World.loadedTileEntityList and friends.
//
// Ownership: a tile entity mapped into a chunk is owned by that chunk. Java also lets tile entities
// keep ticking after they've been unmapped (replaced by another one at the same spot, or placed
// where the block isn't a container). Those are kept alive in `orphans`. Block transmutation
// depends on this.
class WorldManager;
struct TileEntityManager {
	// In Java's list order. Expired entries belonged to unloaded chunks.
	std::vector<std::weak_ptr<TileEntity>> tickableTileEntities;

	// This flag is set while the manager ticks tile entities
	// Tile entity additions are deferred until all tile entities are done ticking
	bool scanning = false;
	std::vector<std::shared_ptr<TileEntity>> pendingAdd;

	// Tile entities that still tick but aren't mapped into any chunk
	std::vector<std::shared_ptr<TileEntity>> orphans;

	// Initialize a tile entity into the world
	void InitializeTileEntity(const std::shared_ptr<TileEntity>& _tileEntity) {
		if (_tileEntity->canTick) {
			tickableTileEntities.push_back(_tileEntity);
		}
	}

	// Keep an unmapped tile entity ticking
	void KeepAlive(const std::shared_ptr<TileEntity>& _tileEntity) {
		if (!_tileEntity->canTick || _tileEntity->invalid)
			return;
		if (std::find(orphans.begin(), orphans.end(), _tileEntity) == orphans.end())
			orphans.push_back(_tileEntity);
	}

	// loadedTileEntityList.remove(te)
	void Unlist(const TileEntity* _tileEntity) {
		std::erase_if(tickableTileEntities, [&](const std::weak_ptr<TileEntity>& _wp) {
			auto te = _wp.lock();
			return !te || te.get() == _tileEntity;
		});
		std::erase_if(orphans, [&](const std::shared_ptr<TileEntity>& _te) { return _te.get() == _tileEntity; });
	}

	bool IsListed(const TileEntity* _tileEntity) const {
		for (const auto& wp : tickableTileEntities)
			if (auto te = wp.lock(); te.get() == _tileEntity)
				return true;
		return false;
	}

	void TickTileEntities(WorldManager& _world);
};
