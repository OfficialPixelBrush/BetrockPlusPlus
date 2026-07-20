/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "BS_thread_pool.hpp"
#include "region.h"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct SnapshotContainer {
	std::shared_ptr<Chunk> chunkSnapshot;
	std::shared_ptr<const std::vector<Tag>> entitySnapshot;
};

struct WorldManager;

struct RegionManager {
	BS::thread_pool<> iopool{ 2 };

	std::mutex saveQueueMutex;
	std::vector<SnapshotContainer> saveQueue;

	std::mutex outChunksMutex;
	std::unordered_map<Int32_2, std::shared_ptr<Chunk>> outChunks;

	// As much as I hate to do this it makes my job easier
	WorldManager* world = nullptr;

	~RegionManager();

	bool initialize(const std::string& _folderPath);
	bool release();

	// Does this region file exist on the disk?
	bool regionExists(Int32_2 _rpos);

	// Has this chunk been saved to a region file yet?
	bool chunkExists(Int32_2 _cpos);

	// Creates a new region file
	bool createRegion(Int32_2 _rpos);

	// Serialize and save a chunk to a region
	void saveChunk(std::shared_ptr<Chunk> _chunk, bool _unloadEntities = false);

	// Queue a chunk to be loaded from disk
	void loadChunk(Int32_2 _cpos);

	// Returns nullptr until chunk is done loading
	std::shared_ptr<Chunk> getChunk(Int32_2 _cpos);

	void pumpPipeline();

	// Flush all pending saves synchronously
	void flushAll();

	// Loads a region into cache, creating the file if needed
	std::shared_ptr<Region> loadRegion(Int32_2 _rpos);

private:
	bool tryMergePendingRegion(std::shared_ptr<Region>& _region);
	bool createRegionOnCache(Int2 _rpos);

	std::vector<std::shared_ptr<Region>> pendingRegions;
	std::shared_ptr<Region> regionCache[8];
	int cacheIndex = 0;
	std::string m_folderPath; // Path to where all the regions get dumped
};