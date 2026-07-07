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
struct TileEntityManager {
	std::vector<std::weak_ptr<TileEntity>> tickableTileEntities;

	// Initialize a tile entity into the world
	void initializeTileEntity(const std::shared_ptr<TileEntity>& tileEntity) {
		if (tileEntity->m_canTick) {
			tickableTileEntities.push_back(tileEntity);
		}
	}

	void tickTileEntities() {
		std::erase_if(tickableTileEntities, [](const std::weak_ptr<TileEntity>& wp) {
			auto te = wp.lock();
			if (!te)
				return true;
			te->tick();
			return false;
		});
	}
};