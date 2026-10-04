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

	scanning = true;
	for (const std::weak_ptr<TileEntity>& wp : tickableTileEntitiesCopy) {
		if (auto te = wp.lock()) {
			if (!te->invalid)
				te->Tick(_world);
		}
	}
	scanning = false;

	// Attach everything that was created during the scan
	std::vector<std::shared_ptr<TileEntity>> added;
	added.swap(pendingAdd);
	for (auto& te : added) {
		if (te->invalid)
			continue;

		Chunk* chunk = _world.GetChunkRaw({ te->position.x >> 4, te->position.z >> 4 });

		bool blockAccepts = te->type != TileType::PISTON_MOVING ||
		                    _world.GetBlockId(te->position) == BLOCK_PISTON_MOVING;

		if (chunk && blockAccepts) {
			_world.RemoveTileEntity(te->position); // Replace whatever was there
			_world.CreateTileEntity(te);
		} else {
			te->chunk = chunk;
			InitializeTileEntity(te);
			orphans.push_back(te);
		}
	}

	std::erase_if(orphans, [](const std::shared_ptr<TileEntity>& _te) { return _te->invalid; });
	std::erase_if(tickableTileEntities, [](const std::weak_ptr<TileEntity>& _wp) {
		auto te = _wp.lock();
		return !te || te->invalid;
	});
}
