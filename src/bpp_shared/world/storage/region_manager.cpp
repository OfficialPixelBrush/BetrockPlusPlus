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
		GlobalLogger().m_error << "Tried to initialize region manager with an invalid directory!\n";
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

	m_world = nullptr;

	return true;
}

bool RegionManager::regionExists(Int32_2 rpos) {
	return std::filesystem::exists(m_folderPath + "/" + regionPositionToFileName(rpos));
}

bool RegionManager::chunkExists(Int32_2 cpos) {
	if (!regionExists({ cpos.m_x >> 5, cpos.m_z >> 5 }))
		return false;
	auto region = loadRegion({ cpos.m_x >> 5, cpos.m_z >> 5 });
	if (!region)
		return false;
	return region->chunkExists({ cpos.m_x & 31, cpos.m_z & 31 });
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
		std::lock_guard lk(m_outChunksMutex);
		m_outChunks.erase(chunk->m_cpos);
	}

	// Make a snapshot
	auto snapshot = std::make_shared<Chunk>();
	snapshot->m_cpos = chunk->m_cpos;
	snapshot->m_isTerrainPopulated = chunk->m_isTerrainPopulated;
	snapshot->m_isModified = chunk->m_isModified;
	snapshot->m_spawnChunk = chunk->m_spawnChunk;
	snapshot->m_state.store(chunk->m_state.load(std::memory_order_acquire));
	snapshot->m_inUse.store(false);
	std::memcpy(snapshot->m_blocks, chunk->m_blocks, sizeof(chunk->m_blocks));
	std::memcpy(snapshot->m_lightNibble, chunk->m_lightNibble, sizeof(chunk->m_lightNibble));
	std::memcpy(snapshot->m_nibbleBlockMeta, chunk->m_nibbleBlockMeta, sizeof(chunk->m_nibbleBlockMeta));
	std::memcpy(snapshot->m_heightMap, chunk->m_heightMap, sizeof(chunk->m_heightMap));
	std::memcpy(snapshot->m_temperature, chunk->m_temperature, sizeof(chunk->m_temperature));
	std::memcpy(snapshot->m_humidity, chunk->m_humidity, sizeof(chunk->m_humidity));
	snapshot->m_tileEntities = chunk->m_tileEntities;

	// Entities are wrapped in a shared_ptr<const vector>
	auto entities = std::make_shared<const std::vector<Tag>>(
	    m_world->m_entityManager.collectEntitiesForSave(snapshot->m_cpos, unloadEntities));

	SnapshotContainer container{ std::move(snapshot), std::move(entities) };

	std::lock_guard lk(m_saveQueueMutex);
	m_saveQueue.push_back(std::move(container));
}

void RegionManager::loadChunk(Int32_2 cpos) {
	Int32_2 rpos{ cpos.m_x >> 5, cpos.m_z >> 5 };
	if (!regionExists(rpos))
		return;
	auto region = loadRegion(rpos); // shared_ptr keeps Region alive for the task
	if (!region)
		return;
	m_iopool.detach_task([cpos, region, this]() {
		auto chunk = region->GetChunk(cpos); // blocks until region is free
		if (!chunk)
			return;
		std::lock_guard lk(m_outChunksMutex);
		m_outChunks[cpos] = std::move(chunk);
	});
}

std::shared_ptr<Chunk> RegionManager::getChunk(Int32_2 cpos) {
	std::lock_guard lk(m_outChunksMutex);
	auto it = m_outChunks.find(cpos);
	if (it == m_outChunks.end())
		return nullptr;
	auto chunk = std::move(it->second);
	m_outChunks.erase(it);
	return chunk;
}

void RegionManager::pumpPipeline() {
	std::vector<SnapshotContainer> toSave;
	{
		std::lock_guard lk(m_saveQueueMutex);
		toSave.swap(m_saveQueue);
		if (m_saveQueue.empty())
			m_saveQueue.shrink_to_fit();
	}

	std::vector<SnapshotContainer> requeue;
	for (auto& snapshot : toSave) {
		auto& chunk = snapshot.m_chunkSnapshot;
		auto entitySnapshot = snapshot.m_entitySnapshot; // shared_ptr copy
		Int32_2 rpos{ chunk->m_cpos.m_x >> 5, chunk->m_cpos.m_z >> 5 };
		if (!regionExists(rpos))
			createRegion(rpos);
		auto region = loadRegion(rpos); // shared_ptr keeps Region alive for the task
		if (!region) {
			requeue.push_back(std::move(snapshot)); // keep chunk + entities together
			continue;
		}
		auto currentTime = m_world->m_elapsed_ticks;
		m_iopool.detach_task([chunk, region, currentTime, entitySnapshot]() {
			region->AddChunk(chunk, currentTime, entitySnapshot); // Region stays alive via shared_ptr capture
		});
	}

	if (!requeue.empty()) {
		std::lock_guard lk(m_saveQueueMutex);
		m_saveQueue.insert(m_saveQueue.end(), std::make_move_iterator(requeue.begin()),
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
	m_iopool.wait();
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
			GlobalLogger().m_error << "Failed to create region file for " << rpos.m_x << "," << rpos.m_z << "\n";
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