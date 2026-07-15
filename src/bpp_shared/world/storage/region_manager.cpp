/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "region_manager.h"
#include "world/world.h" // Full WorldManager definition needed here for world->elapsed_ticks etc.
#include <filesystem>
#include <fstream>
#include <libdeflate.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

RegionManager::~RegionManager() {
	release();
}

bool RegionManager::initialize(const std::string& folderPath) {
	if (!std::filesystem::is_directory(folderPath)) {
		GlobalLogger().error << "Tried to initialize region manager with an invalid directory!\n";
		return false; // No region folder
	}
	m_folderPath = folderPath;
	return true;
}

bool RegionManager::release() {
	flushAll();

	// Clear regions in cache
	for (int i = 0; i < 8; i++)
		m_regionCache[i].reset();

	// Drop any regions that couldn't fit in cache
	m_pendingRegions.clear();

	// Clear the folder path so the manager can't be accidentally reused
	m_folderPath.clear();

	world = nullptr;

	return true;
}

bool RegionManager::regionExists(Int32_2 rpos) {
	return std::filesystem::exists(m_folderPath + "/" + regionPositionToFileName(rpos));
}

bool RegionManager::chunkExists(Int32_2 cpos) {
	if (!regionExists({ cpos.x >> 5, cpos.z >> 5 }))
		return false;
	auto region = loadRegion({ cpos.x >> 5, cpos.z >> 5 });
	if (!region)
		return false;
	return region->chunkExists({ cpos.x & 31, cpos.z & 31 });
}

bool RegionManager::createRegion(Int32_2 rpos) {
	std::string path = m_folderPath + "/" + regionPositionToFileName(rpos);
	std::filesystem::create_directories(m_folderPath);
	std::ofstream file(path, std::ios::binary);
	if (!file)
		return false;
	std::vector<char> zeros(8192, 0); // 2 sectors for the header
	file.write(zeros.data(), zeros.size());
	file.close();       // explicit close before FileHandle opens it
	return file.good(); // catch write failures too
}

void RegionManager::saveChunk(std::shared_ptr<Chunk> chunk, bool unloadEntities) {
	{
		std::lock_guard lk(outChunksMutex);
		outChunks.erase(chunk->cpos);
	}

	// Make a snapshot
	auto snapshot = std::make_shared<Chunk>();
	snapshot->cpos = chunk->cpos;
	snapshot->isTerrainPopulated = chunk->isTerrainPopulated;
	snapshot->isModified = chunk->isModified;
	snapshot->spawnChunk = chunk->spawnChunk;
	snapshot->state.store(chunk->state.load(std::memory_order_acquire));
	snapshot->inUse.store(false);
	std::memcpy(snapshot->blocks, chunk->blocks, sizeof(chunk->blocks));
	std::memcpy(snapshot->lightNibble, chunk->lightNibble, sizeof(chunk->lightNibble));
	std::memcpy(snapshot->nibbleBlockMeta, chunk->nibbleBlockMeta, sizeof(chunk->nibbleBlockMeta));
	std::memcpy(snapshot->heightMap, chunk->heightMap, sizeof(chunk->heightMap));
	std::memcpy(snapshot->temperature, chunk->temperature, sizeof(chunk->temperature));
	std::memcpy(snapshot->humidity, chunk->humidity, sizeof(chunk->humidity));
	snapshot->tileEntities = chunk->tileEntities;

	// Entities are wrapped in a shared_ptr<const vector>
	auto entities = std::make_shared<const std::vector<Tag>>(
	    world->entityManager.collectEntitiesForSave(snapshot->cpos, unloadEntities));

	SnapshotContainer container{ std::move(snapshot), std::move(entities) };

	std::lock_guard lk(saveQueueMutex);
	saveQueue.push_back(std::move(container));
}

void RegionManager::loadChunk(Int32_2 cpos) {
	Int32_2 rpos{ cpos.x >> 5, cpos.z >> 5 };
	if (!regionExists(rpos))
		return;
	auto region = loadRegion(rpos); // shared_ptr keeps Region alive for the task
	if (!region)
		return;
	iopool.detach_task([cpos, region, this]() {
		auto chunk = region->GetChunk(cpos); // blocks until region is free
		if (!chunk)
			return;
		std::lock_guard lk(outChunksMutex);
		outChunks[cpos] = std::move(chunk);
	});
}

std::shared_ptr<Chunk> RegionManager::getChunk(Int32_2 cpos) {
	std::lock_guard lk(outChunksMutex);
	auto it = outChunks.find(cpos);
	if (it == outChunks.end())
		return nullptr;
	auto chunk = std::move(it->second);
	outChunks.erase(it);
	return chunk;
}

void RegionManager::pumpPipeline() {
	std::vector<SnapshotContainer> toSave;
	{
		std::lock_guard lk(saveQueueMutex);
		toSave.swap(saveQueue);
		if (saveQueue.empty())
			saveQueue.shrink_to_fit();
	}

	std::vector<SnapshotContainer> requeue;
	for (auto& snapshot : toSave) {
		auto& chunk = snapshot.chunkSnapshot;
		auto entitySnapshot = snapshot.entitySnapshot; // shared_ptr copy
		Int32_2 rpos{ chunk->cpos.x >> 5, chunk->cpos.z >> 5 };
		if (!regionExists(rpos))
			createRegion(rpos);
		auto region = loadRegion(rpos); // shared_ptr keeps Region alive for the task
		if (!region) {
			requeue.push_back(std::move(snapshot)); // keep chunk + entities together
			continue;
		}
		auto currentTime = world->elapsed_ticks;
		iopool.detach_task([chunk, region, currentTime, entitySnapshot]() {
			region->AddChunk(chunk, currentTime, entitySnapshot); // Region stays alive via shared_ptr capture
		});
	}

	if (!requeue.empty()) {
		std::lock_guard lk(saveQueueMutex);
		saveQueue.insert(saveQueue.end(), std::make_move_iterator(requeue.begin()),
		                 std::make_move_iterator(requeue.end()));
	}

	// Try to merge any pending regions that couldn't fit before
	for (auto it = m_pendingRegions.begin(); it != m_pendingRegions.end();) {
		if (tryMergePendingRegion(*it))
			it = m_pendingRegions.erase(it);
		else
			++it;
	}
}

void RegionManager::flushAll() {
	pumpPipeline();
	iopool.wait();
}

std::shared_ptr<Region> RegionManager::loadRegion(Int32_2 rpos) {
	// Check cache first
	for (int i = 0; i < 8; i++) {
		if (m_regionCache[i] && m_regionCache[i]->m_rpos == rpos)
			return m_regionCache[i];
	}

	// Also check regions awaiting merge
	for (auto& pending : m_pendingRegions) {
		if (pending->m_rpos == rpos)
			return pending;
	}

	if (!regionExists(rpos)) {
		if (!createRegion(rpos)) {
			GlobalLogger().error << "Failed to create region file for " << rpos.x << "," << rpos.z << "\n";
			return nullptr;
		}
	}

	if (createRegionOnCache(rpos)) {
		for (int i = 0; i < 8; i++) {
			if (m_regionCache[i] && m_regionCache[i]->m_rpos == rpos)
				return m_regionCache[i];
		}
	}

	return nullptr; // all 8 slots still busy
}

bool RegionManager::tryMergePendingRegion(std::shared_ptr<Region>& region) {
	for (int i = 0; i < 8; i++) {
		if (m_regionCache[i] && m_regionCache[i]->m_rpos == region->m_rpos)
			return true; // already cached
	}
	for (int i = 0; i < 8; i++) {
		if (!m_regionCache[i]) {
			m_regionCache[i] = region;
			return true;
		}
		// Evict slot if no IO task currently holds a reference to it.
		if (m_regionCache[i].use_count() == 1) {
			m_regionCache[i] = region;
			return true;
		}
	}
	return false; // all 8 slots actively in use
}

bool RegionManager::createRegionOnCache(Int2 rpos) {
	auto region = std::make_shared<Region>(rpos, m_folderPath);
	if (!tryMergePendingRegion(region)) {
		m_pendingRegions.push_back(std::move(region));
		return false;
	}
	return true;
}