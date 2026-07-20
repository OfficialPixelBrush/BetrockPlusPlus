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
	Release();
}

bool RegionManager::Initialize(const std::string& _folderPath) {
	if (!std::filesystem::is_directory(_folderPath)) {
		GlobalLogger().error << "Tried to initialize region manager with an invalid directory!\n";
		return false; // No region folder
	}
	mFolderPath = _folderPath;
	return true;
}

bool RegionManager::Release() {
	FlushAll();

	// Clear regions in cache
	for (int i = 0; i < 8; i++)
		regionCache[i].reset();

	// Drop any regions that couldn't fit in cache
	pendingRegions.clear();

	// Clear the folder path so the manager can't be accidentally reused
	mFolderPath.clear();

	world = nullptr;

	return true;
}

bool RegionManager::RegionExists(Int32_2 _rpos) {
	return std::filesystem::exists(mFolderPath + "/" + RegionPositionToFileName(_rpos));
}

bool RegionManager::ChunkExists(Int32_2 _cpos) {
	if (!RegionExists({ _cpos.x >> 5, _cpos.z >> 5 }))
		return false;
	auto region = LoadRegion({ _cpos.x >> 5, _cpos.z >> 5 });
	if (!region)
		return false;
	return region->ChunkExists({ _cpos.x & 31, _cpos.z & 31 });
}

bool RegionManager::CreateRegion(Int32_2 _rpos) {
	std::string path = mFolderPath + "/" + RegionPositionToFileName(_rpos);
	std::filesystem::create_directories(mFolderPath);
	std::ofstream file(path, std::ios::binary);
	if (!file)
		return false;
	std::vector<char> zeros(8192, 0); // 2 sectors for the header
	file.write(zeros.data(), zeros.size());
	file.close();       // explicit close before FileHandle opens it
	return file.good(); // catch write failures too
}

void RegionManager::SaveChunk(std::shared_ptr<Chunk> _chunk, bool _unloadEntities) {
	{
		std::lock_guard lk(outChunksMutex);
		outChunks.erase(_chunk->cpos);
	}

	// Make a snapshot
	auto snapshot = std::make_shared<Chunk>();
	snapshot->cpos = _chunk->cpos;
	snapshot->isTerrainPopulated = _chunk->isTerrainPopulated;
	snapshot->isModified = _chunk->isModified;
	snapshot->spawnChunk = _chunk->spawnChunk;
	snapshot->state.store(_chunk->state.load(std::memory_order_acquire));
	snapshot->inUse.store(false);
	std::memcpy(snapshot->blocks, _chunk->blocks, sizeof(_chunk->blocks));
	std::memcpy(snapshot->lightNibble, _chunk->lightNibble, sizeof(_chunk->lightNibble));
	std::memcpy(snapshot->nibbleBlockMeta, _chunk->nibbleBlockMeta, sizeof(_chunk->nibbleBlockMeta));
	std::memcpy(snapshot->heightMap, _chunk->heightMap, sizeof(_chunk->heightMap));
	std::memcpy(snapshot->temperature, _chunk->temperature, sizeof(_chunk->temperature));
	std::memcpy(snapshot->humidity, _chunk->humidity, sizeof(_chunk->humidity));
	snapshot->tileEntities = _chunk->tileEntities;

	// Entities are wrapped in a shared_ptr<const vector>
	auto entities = std::make_shared<const std::vector<Tag>>(
	    world->entityManager.CollectEntitiesForSave(snapshot->cpos, _unloadEntities));

	SnapshotContainer container{ std::move(snapshot), std::move(entities) };

	std::lock_guard lk(saveQueueMutex);
	saveQueue.push_back(std::move(container));
}

void RegionManager::LoadChunk(Int32_2 _cpos) {
	Int32_2 rpos{ _cpos.x >> 5, _cpos.z >> 5 };
	if (!RegionExists(rpos))
		return;
	auto region = LoadRegion(rpos); // shared_ptr keeps Region alive for the task
	if (!region)
		return;
	iopool.detach_task([_cpos, region, this]() {
		auto chunk = region->GetChunk(_cpos); // blocks until region is free
		if (!chunk)
			return;
		std::lock_guard lk(outChunksMutex);
		outChunks[_cpos] = std::move(chunk);
	});
}

std::shared_ptr<Chunk> RegionManager::GetChunk(Int32_2 _cpos) {
	std::lock_guard lk(outChunksMutex);
	auto it = outChunks.find(_cpos);
	if (it == outChunks.end())
		return nullptr;
	auto chunk = std::move(it->second);
	outChunks.erase(it);
	return chunk;
}

void RegionManager::PumpPipeline() {
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
		if (!RegionExists(rpos))
			CreateRegion(rpos);
		auto region = LoadRegion(rpos); // shared_ptr keeps Region alive for the task
		if (!region) {
			requeue.push_back(std::move(snapshot)); // keep chunk + entities together
			continue;
		}
		auto currentTime = world->elapsedTicks;
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
	for (auto it = pendingRegions.begin(); it != pendingRegions.end();) {
		if (TryMergePendingRegion(*it))
			it = pendingRegions.erase(it);
		else
			++it;
	}
}

void RegionManager::FlushAll() {
	PumpPipeline();
	iopool.wait();
}

std::shared_ptr<Region> RegionManager::LoadRegion(Int32_2 _rpos) {
	// Check cache first
	for (int i = 0; i < 8; i++) {
		if (regionCache[i] && regionCache[i]->rpos == _rpos)
			return regionCache[i];
	}

	// Also check regions awaiting merge
	for (auto& pending : pendingRegions) {
		if (pending->rpos == _rpos)
			return pending;
	}

	if (!RegionExists(_rpos)) {
		if (!CreateRegion(_rpos)) {
			GlobalLogger().error << "Failed to create region file for " << _rpos.x << "," << _rpos.z << "\n";
			return nullptr;
		}
	}

	if (CreateRegionOnCache(_rpos)) {
		for (int i = 0; i < 8; i++) {
			if (regionCache[i] && regionCache[i]->rpos == _rpos)
				return regionCache[i];
		}
	}

	return nullptr; // all 8 slots still busy
}

bool RegionManager::TryMergePendingRegion(std::shared_ptr<Region>& _region) {
	for (int i = 0; i < 8; i++) {
		if (regionCache[i] && regionCache[i]->rpos == _region->rpos)
			return true; // already cached
	}
	for (int i = 0; i < 8; i++) {
		if (!regionCache[i]) {
			regionCache[i] = _region;
			return true;
		}
		// Evict slot if no IO task currently holds a reference to it.
		if (regionCache[i].use_count() == 1) {
			regionCache[i] = _region;
			return true;
		}
	}
	return false; // all 8 slots actively in use
}

bool RegionManager::CreateRegionOnCache(Int2 _rpos) {
	auto region = std::make_shared<Region>(_rpos, mFolderPath);
	if (!TryMergePendingRegion(region)) {
		pendingRegions.push_back(std::move(region));
		return false;
	}
	return true;
}