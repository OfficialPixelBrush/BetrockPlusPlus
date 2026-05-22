/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "helpers/file_handle.h"
#include "nbt/nbt.h"
#include "java_math.h"
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <libdeflate.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif

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

	bool initialize(const std::string& folderPath) {
		if (!std::filesystem::is_directory(folderPath))
			return false; // No region folder
		return true;
	}

	// This is just a cache of file handles that is lazily swapped out when it runs out
	std::unordered_map<Int32_2, std::shared_ptr<Region>> regionCache;

	// Does this region file exist on the disk?
	bool regionExists(Int32_2 rpos);

	// Has this chunk been saved to a region file yet?
	bool chunkExists(Int32_2 cpos);

	// Creates a new region file
	bool createRegion(Int32_2 rpos);

	// Serialize and save a chunk to a region
	void saveChunk(std::shared_ptr<Chunk> chunk);

	void pumpPipeline();

	// Creates a region if it doesn't already exist, then returns it.
	std::shared_ptr<Region> loadRegion(Int32_2 rpos);

	// Returns nullptr until chunk is done loading
	std::shared_ptr<Chunk> getChunk(Int32_2 cpos);

private:
	int cacheSize = 8; // 8 Region files at a time
	std::string folderPath; // Path to where all the regions get dumped
};