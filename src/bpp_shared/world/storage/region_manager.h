/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
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

	std::mutex saveQueueMutex;
	std::vector<std::shared_ptr<Chunk>> saveQueue;

	std::mutex outChunksMutex;
	std::unordered_map<Int32_2, std::shared_ptr<Chunk>> outChunks;

	bool initialize(const std::string& folderPath) {
		if (!std::filesystem::is_directory(folderPath)) {
			GlobalLogger().error << "Tried to initialize region manager with an invalid directory!\n";
			return false; // No region folder
		}
		m_folderPath = folderPath;
		return true;
	}

	// Does this region file exist on the disk?
	bool regionExists(Int32_2 rpos) {
		return std::filesystem::exists(m_folderPath + "/" + regionPositionToFileName(rpos));
	}

	// Has this chunk been saved to a region file yet?
	bool chunkExists(Int32_2 cpos) {
		if (!regionExists({ cpos.x >> 5, cpos.z >> 5 })) return false;
		auto region = loadRegion({ cpos.x >> 5, cpos.z >> 5 });
		if (!region) return false;
		return region->chunkExists({ cpos.x & 31, cpos.z & 31 });
	}

	// Creates a new region file
	bool createRegion(Int32_2 rpos) {
		std::string path = m_folderPath + "/" + regionPositionToFileName(rpos);
		std::filesystem::create_directories(m_folderPath);
		std::ofstream file(path, std::ios::binary);
		if (!file) return false;
		std::vector<char> zeros(8192, 0); // 2 sectors for the header
		file.write(zeros.data(), zeros.size());
		file.close();                          // explicit close before FileHandle opens it
		return file.good();                    // catch write failures too
	}

	// Serialize and save a chunk to a region
	void saveChunk(std::shared_ptr<Chunk> chunk) {
		{
			std::lock_guard lk(outChunksMutex);
			outChunks.erase(chunk->cpos);
		}

		// Make a snapshot because I kept crashing..
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
		std::lock_guard lk(saveQueueMutex);
		saveQueue.push_back(std::move(snapshot));
	}

	// Queue a chunk to be loaded from disk
	void loadChunk(Int32_2 cpos) {
		Int32_2 rpos{ cpos.x >> 5, cpos.z >> 5 };
		if (!regionExists(rpos)) return;
		auto region = loadRegion(rpos); // shared_ptr keeps Region alive for the task
		if (!region) return;
		iopool.detach_task([cpos, region, this]() {
			auto chunk = region->GetChunk(cpos);  // blocks until region is free
			if (!chunk) return;
			std::lock_guard lk(outChunksMutex);
			outChunks[cpos] = std::move(chunk);
			});
	}

	// Returns nullptr until chunk is done loading
	std::shared_ptr<Chunk> getChunk(Int32_2 cpos) {
		std::lock_guard lk(outChunksMutex);
		auto it = outChunks.find(cpos);
		if (it == outChunks.end()) return nullptr;
		auto chunk = std::move(it->second);
		outChunks.erase(it);
		return chunk;
	}

	void pumpPipeline() {
		std::vector<std::shared_ptr<Chunk>> toSave;
		{
			std::lock_guard lk(saveQueueMutex);
			toSave.swap(saveQueue);
			if (saveQueue.empty()) saveQueue.shrink_to_fit();
		}

		std::vector<std::shared_ptr<Chunk>> requeue;
		for (auto& chunk : toSave) {
			Int32_2 rpos{ chunk->cpos.x >> 5, chunk->cpos.z >> 5 };
			if (!regionExists(rpos)) createRegion(rpos);
			auto region = loadRegion(rpos); // shared_ptr keeps Region alive for the task
			if (!region) {
				requeue.push_back(chunk);
				continue;
			}
			iopool.detach_task([chunk, region]() {
				region->AddChunk(chunk);  // Region stays alive via shared_ptr capture
				});
		}

		if (!requeue.empty()) {
			std::lock_guard lk(saveQueueMutex);
			saveQueue.insert(saveQueue.end(), requeue.begin(), requeue.end());
		}

		// Try to merge any pending regions that couldn't fit before
		for (auto it = m_pendingRegions.begin(); it != m_pendingRegions.end(); ) {
			if (tryMergePendingRegion(*it))
				it = m_pendingRegions.erase(it);
			else
				++it;
		}
	}

	// Flush all pending saves synchronously � call on shutdown
	void flushAll() {
		pumpPipeline();
		iopool.wait();
	}

	// Creates a region if it doesn't already exist, then returns it.
	std::shared_ptr<Region> loadRegion(Int32_2 rpos) {
		// Check cache first
		for (int i = 0; i < 8; i++) {
			if (m_regionCache[i] && m_regionCache[i]->m_rpos == rpos)
				return m_regionCache[i];
		}

		// Cache miss, so we wait for all in-flight IO to finish before we potentially evict a slot, preventing two Region objects opening the same file (BAD!)
		iopool.wait();

		// Check cache again after wait
		for (int i = 0; i < 8; i++) {
			if (m_regionCache[i] && m_regionCache[i]->m_rpos == rpos)
				return m_regionCache[i];
		}

		if (!regionExists(rpos)) {
			if (!createRegion(rpos)) {
				GlobalLogger().error << "Failed to create region file for "
					<< rpos.x << "," << rpos.z << "\n";
				return nullptr;
			}
		}

		createRegionOnCache(rpos);

		for (int i = 0; i < 8; i++) {
			if (m_regionCache[i] && m_regionCache[i]->m_rpos == rpos)
				return m_regionCache[i];
		}

		return nullptr; // all 8 slots still busy (shouldn't happen after wait)
	}

private:
	bool tryMergePendingRegion(std::shared_ptr<Region>& region) {
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

	void createRegionOnCache(Int2 rpos) {
		auto region = std::make_shared<Region>(rpos, m_folderPath);
		if (!tryMergePendingRegion(region))
			m_pendingRegions.push_back(std::move(region));
	}

	std::vector<std::shared_ptr<Region>> m_pendingRegions;
	std::shared_ptr<Region> m_regionCache[8];
	int m_cacheIndex = 0;
	std::string m_folderPath; // Path to where all the regions get dumped
};