/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "BS_thread_pool.hpp"
#include "region.h"
#include <atomic>
#include <shared_mutex>
#include <unordered_set>

struct RegionManager {
	BS::thread_pool<> iopool{ 2 };
	std::mutex saveMutex;
	std::mutex cacheMutex;
	std::shared_mutex outMutex;
	std::vector<std::shared_ptr<Chunk>> saveQueue;
	std::unordered_map<Int32_2, std::shared_ptr<Chunk>> outChunks;
	std::unordered_map<Int32_2, std::shared_ptr<Region>> regionCache;

	bool regionExists(Int32_2 rpos);
	bool chunkExists(Int32_2 cpos);
	bool createRegion(Int32_2 rpos);
	void saveChunk(std::shared_ptr<Chunk> chunk);
	void pumpPipeline();
	// Creates a region if it doesn't already exist, then returns it.
	std::shared_ptr<Region> loadRegion(Int32_2 rpos);
	// Returns nullptr until chunk is done loading
	std::shared_ptr<Chunk> getChunk(Int32_2 cpos);
};