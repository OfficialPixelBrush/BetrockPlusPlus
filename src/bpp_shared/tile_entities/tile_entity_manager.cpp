/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "tile_entity_manager.h"
#include "world/world.h"
#include <algorithm>
#include <unordered_set>

// The tile entity half of World.updateEntities
void TileEntityManager::TickTileEntities(WorldManager& _world) {
	// Snapshot for iteration. Nothing gets added to the live list mid-scan (additions are deferred),
	// but chunk loads could still register tile entities
	std::vector<std::weak_ptr<TileEntity>> tickableTileEntitiesCopy = tickableTileEntities;
	std::unordered_set<const TileEntity*> finished;

	scanning = true;
	for (const std::weak_ptr<TileEntity>& wp : tickableTileEntitiesCopy) {
		auto te = wp.lock();
		if (!te)
			continue; // Its chunk unloaded

		Chunk* chunk = _world.GetChunkRaw({ te->position.x >> 4, te->position.z >> 4 });
		if (te->chunk != chunk) {
			const bool isOrphan = std::find(orphans.begin(), orphans.end(), te) != orphans.end();
			if (isOrphan) {
				// Its chunk unloaded or was reloaded, so its chunk pointer is dangling. Drop it
				te->invalid = true;
				finished.insert(te.get());
				continue;
			}
			// Something else (like a pending save) is keeping an unloaded chunk's tile entity alive.
			// Leave it alone; invalidating it would drop it from that save
			if (!chunk)
				continue;
			te->chunk = chunk;
		}

		if (!te->invalid)
			te->Tick(_world);

		if (te->invalid) {
			finished.insert(te.get());
			// Java unmaps whatever is at this position, which isn't necessarily this tile entity.
			// An orphan finishing here takes out the tile entity that replaced it.
			_world.RemoveIndexedTileEntity(te->position);
		}
	}
	scanning = false;

	std::erase_if(tickableTileEntities, [&](const std::weak_ptr<TileEntity>& _wp) {
		auto te = _wp.lock();
		return !te || finished.contains(te.get());
	});

	// Attach everything that was created during the scan
	std::vector<std::shared_ptr<TileEntity>> added;
	added.swap(pendingAdd);
	for (auto& te : added) {
		if (te->invalid)
			continue;
		if (!IsListed(te.get()))
			InitializeTileEntity(te);
		// Maps it if the block is a container, otherwise it's kept ticking as an orphan
		_world.IndexTileEntity(te);
	}

	std::erase_if(orphans, [](const std::shared_ptr<TileEntity>& _te) { return _te->invalid; });
}
