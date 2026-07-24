/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "world/storage/region_manager.h"
#include "world/storage/save_manager.h"
#include "helpers/file_handle.h"
#include "nbt/nbt.h"
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

namespace Utilities {

// Creates a temp directory and deletes it if it already exists. Returns false on failure.
inline bool recreateTempDir(const fs::path& _dir) {
	std::error_code ec;
	fs::remove_all(_dir, ec);
	if (ec) {
		GlobalLogger().error << "Failed to remove directory: " << ec.message() << '\n';
		return false;
	}
	fs::create_directories(_dir, ec);
	if (ec) {
		GlobalLogger().error << "Failed to create directory: " << ec.message() << '\n';
		return false;
	}
	return true;
}

// Converts an old betrock server world to mcregion
inline bool convertBetrockServerLevel(std::string& _dir) {
	Blocks::RegisterAll();
	RegionManager regionManager;
	SaveManager saveManager;
	WorldManager world;
	recreateTempDir("ConvertedWorld");

	saveManager.Initialize("ConvertedWorld", true);
	saveManager.CreateNewWorld(/*level data=*/{});

	fs::path srcPlayers = fs::path(_dir) / "players";
	fs::path srcRegion = fs::path(_dir) / "region";

	if (!fs::exists(srcPlayers) || !fs::exists(srcRegion)) {
		GlobalLogger().error << "Invalid Betrock level file!\n";
		return false;
	}

	if (!regionManager.Initialize("ConvertedWorld/region")) {
		GlobalLogger().error << "Failed to initialize overworld region manager for conversion!\n";
		return false;
	}

	fs::copy(srcPlayers, "ConvertedWorld/players", fs::copy_options::recursive | fs::copy_options::overwrite_existing);

	auto BlockIndexToPosition = [](int32_t index) -> Int3 {
		Int3 pos;
		pos.y = index % CHUNK_HEIGHT;
		index /= CHUNK_HEIGHT;

		pos.z = index % CHUNK_WIDTH;
		index /= CHUNK_WIDTH;

		pos.x = index;
		return pos;
	};

	auto decompressChunk = [](const char* compressed_data, size_t compressed_size,
	                          size_t& decompressed_size) -> std::unique_ptr<char[]> {
		struct libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();
		if (!decompressor)
			return nullptr;

		size_t capacity = decompressed_size > 0 ? decompressed_size : 81920;
		for (int attempt = 0; attempt < 6; attempt++) {
			auto decompressed_data = std::make_unique<char[]>(capacity);
			size_t actual_size = 0;
			int32_t result = libdeflate_zlib_decompress(decompressor, compressed_data, compressed_size,
			                                            decompressed_data.get(), capacity, &actual_size);
			if (result == LIBDEFLATE_SUCCESS) {
				decompressed_size = actual_size;
				libdeflate_free_decompressor(decompressor);
				return decompressed_data;
			}
			if (result != LIBDEFLATE_INSUFFICIENT_SPACE)
				break;     // real error, don't keep retrying
			capacity *= 2; // grow and try again
		}
		libdeflate_free_decompressor(decompressor);
		return nullptr;
	};

	auto loadOldFormat = [&](FileHandle& cFile) -> std::shared_ptr<Chunk> {
		auto& chunkFile = cFile.Get();

		// Get the length of the file
		chunkFile.seekg(0, std::ios::end);
		std::streamsize size = chunkFile.tellg();
		chunkFile.seekg(0, std::ios::beg);

		std::vector<char> buffer(size);
		chunkFile.read(buffer.data(), size);
		char* compressedChunk = buffer.data();

		size_t compressedSize = size;
		size_t decompressedSize = 0;

		auto chunkData = decompressChunk(compressedChunk, compressedSize, decompressedSize);

		if (!chunkData) return nullptr;

		std::shared_ptr<Chunk> c = std::make_shared<Chunk>();
		size_t blockDataSize = CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT;
		size_t nibbleDataSize = CHUNK_WIDTH * CHUNK_WIDTH * (CHUNK_HEIGHT / 2);
		for (size_t i = 0; i < decompressedSize; i++) {
			if (i < blockDataSize) {
				// Block Data
				c->SetBlock(BlockIndexToPosition(i), BlockType(chunkData[i]));
			} else if (
			    // Metadata
			    i >= blockDataSize && i < blockDataSize + nibbleDataSize) {
				c->SetMeta(BlockIndexToPosition((i % nibbleDataSize) * 2), chunkData[i] & 0xF);
				c->SetMeta(BlockIndexToPosition((i % nibbleDataSize) * 2 + 1), (chunkData[i] >> 4) & 0xF);
			} else if (
			    // Block Light
			    i >= blockDataSize + nibbleDataSize && i < blockDataSize + (nibbleDataSize * 2)) {
				c->SetBlockLight(BlockIndexToPosition((i % nibbleDataSize) * 2), chunkData[i] & 0xF);
				c->SetBlockLight(BlockIndexToPosition((i % nibbleDataSize) * 2 + 1), (chunkData[i] >> 4) & 0xF);
			} else if (
			    // Sky Light
			    i >= blockDataSize + (nibbleDataSize * 2) && i < blockDataSize + (nibbleDataSize * 3)) {
				c->SetSkyLight(BlockIndexToPosition((i % nibbleDataSize) * 2), chunkData[i] & 0xF);
				c->SetSkyLight(BlockIndexToPosition((i % nibbleDataSize) * 2 + 1), (chunkData[i] >> 4) & 0xF);
			}
		}
		c->isTerrainPopulated = true;
		c->isModified = true;

		return std::move(c);
	};

	auto loadV2Format = [&](FileHandle& cFile) -> std::shared_ptr<Chunk> {
		auto& chunkFile = cFile.Get();
		
		// Get the length of the file
		chunkFile.seekg(0, std::ios::end);
		std::streamsize size = chunkFile.tellg();
		chunkFile.seekg(0, std::ios::beg);

		std::vector<char> buffer(size);
		chunkFile.read(buffer.data(), size);
		char* compressedChunk = buffer.data();

		size_t compressedSize = size;
		size_t decompressedSize = 81920 * 2;

		auto chunkData = decompressChunk(compressedChunk, compressedSize, decompressedSize);
		uint8_t* _pdata = reinterpret_cast<uint8_t*>(chunkData.get());

		auto nbt = NBTParser(_pdata, decompressedSize);

		const size_t blockDataSize = (CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT);
		const size_t nibbleDataSize = (CHUNK_WIDTH * CHUNK_WIDTH * (CHUNK_HEIGHT / 2));

		std::shared_ptr<Chunk> c = std::make_shared<Chunk>();

		auto& root = nbt.root;
		auto& level = root.Get("Level");
		auto& blocksTag = level.Get("Blocks");
		auto& metaTag = level.Get("Data");
		auto& metaData = metaTag.byteArray;

		// Block ids
		for (int i = 0; i < blockDataSize; i++) {
			c->SetBlock(BlockIndexToPosition(i), BlockType(blocksTag.byteArray[i]));
		}

		// Metadata
		for (int i = 0; i < nibbleDataSize; i++) {
			c->SetMeta(BlockIndexToPosition(i * 2), (metaData[i]) & 0xF);
			c->SetMeta(BlockIndexToPosition(i * 2 + 1), (metaData[i] >> 4) & 0xF);
		}

		c->isTerrainPopulated = true;
		c->isModified = true;
		c->GenerateSkylightMap();

		return std::move(c);
	};

	std::vector<Int32_2> chunkCoords;
	for (const auto& entry : fs::directory_iterator(srcRegion)) {
		const fs::path& regionPath = entry.path();
		// Do the old format chunks first
		if (regionPath.extension() == ".cnk") {
			const std::string filename = entry.path().filename().string();
			int rx, rz;

			// Does this have a valid name?
			if (std::sscanf(filename.c_str(), "%d,%d.cnk", &rx, &rz) == 2) {
 				chunkCoords.push_back({ rx, rz });
			}
		}
	}

	GlobalLogger().info << "Found " << chunkCoords.size() << " OLD format chunks to convert.\n";

	// Save all our old format chunks
	for (auto& cpos : chunkCoords) {
		auto path = srcRegion / (std::to_string(cpos.x) + "," + std::to_string(cpos.z) + ".cnk");
		FileHandle fileHandle(path);
		auto chunk = loadOldFormat(fileHandle);
		chunk->cpos = { cpos.x, cpos.z };
		world.chunks[chunk->cpos] = chunk;
	}

	GlobalLogger().info << "Converted! Now converting V2 format chunks..\n";

	// V2 chunks
	chunkCoords.clear();
	for (const auto& entry : fs::directory_iterator(srcRegion)) {
		const fs::path& regionPath = entry.path();
		// Do the old format chunks first
		if (regionPath.extension() == ".ncnk") {
			const std::string filename = entry.path().filename().string();
			int rx, rz;

			// Does this have a valid name?
			if (std::sscanf(filename.c_str(), "%d,%d.ncnk", &rx, &rz) == 2) {
				chunkCoords.push_back({ rx, rz });
			}
		}
	}

	GlobalLogger().info << "Found " << chunkCoords.size() << " V2 format chunks to convert.\n";

	// Save all our V2 format chunks
	int i = 0;
	for (auto& cpos : chunkCoords) {
		auto path = srcRegion / (std::to_string(cpos.x) + "," + std::to_string(cpos.z) + ".ncnk");
		FileHandle fileHandle(path);
		auto chunk = loadV2Format(fileHandle);
		chunk->cpos = { cpos.x, cpos.z };
		world.chunks[chunk->cpos] = chunk;
	}
	GlobalLogger().info << "Finishing up..\n";

	// Save
	for (auto& chunk : world.chunks) {
		regionManager.SaveChunk(chunk.second);
	}

	regionManager.FlushAll();
	regionManager.Release();
	GlobalLogger().info << "Done!\n";

	return true;
}

// Cleans up level data since mcr files can become bloated
inline bool cleanLevel(std::string _relPath) {
	SaveManager saveManager;
	RegionManager regionManager;
	RegionManager outRegionManager;

	if (!saveManager.Initialize(_relPath)) {
		GlobalLogger().error << "Failed to initialize save manager for cleaning!\n";
		return false;
	}

	// Overworld first
	{
		if (!regionManager.Initialize(_relPath + "/region")) {
			GlobalLogger().error << "Failed to initialize overworld region manager for cleaning! (Skipping)\n";
		}

		if (!recreateTempDir(_relPath + "/tempRegion")) {
			GlobalLogger().error << "Failed to recreate temp directory!\n";
			return false;
		}

		if (!outRegionManager.Initialize(_relPath + "/tempRegion")) {
			GlobalLogger().error << "Failed to initialize output region manager for cleaning!\n";
			return false;
		}

		// Check what regions exist
		GlobalLogger().info << "Scanning for regions to clean...\n";
		std::vector<Int32_2> regionCoords;
		for (const auto& entry : fs::directory_iterator(_relPath + "/region")) {
			const fs::path& regionPath = entry.path();
			// Is this a valid region file?
			if (regionPath.extension() == ".mcr") {
				// Get the coordinate pair for this region
				const std::string filename = entry.path().filename().string();
				int rx, rz;

				// Does this have a valid name?
				if (std::sscanf(filename.c_str(), "r.%d.%d.mcr", &rx, &rz) == 2) {
					regionCoords.push_back({ rx, rz });
				}
			}
		}

		GlobalLogger().info << "Found " << regionCoords.size() << " regions to clean.\n";

		// Go through each region
		int chunksCleaned = 0;
		int regionCnt = 0;
		for (auto& regionCoord : regionCoords) {
			auto region = regionManager.LoadRegion(regionCoord);
			outRegionManager.CreateRegion(regionCoord);
			auto newRegion = outRegionManager.LoadRegion(regionCoord);
			for (int cx = 0; cx < 32; cx++) {
				for (int cz = 0; cz < 32; cz++) {
					// Load the chunk if it exists
					if (region->ChunkExists({ cx, cz })) {
						chunksCleaned++;
						auto chunk = region->GetChunk({ regionCoord.x * 32 + cx, regionCoord.z * 32 + cz });
						// Save to a new region file
						newRegion->AddChunk(chunk, 0, {});
					}
				}
			}
			regionCnt++;
			GlobalLogger().info << "Cleaned region (Overworld):" << regionCoord << ": " << regionCnt << " / "
			                    << regionCoords.size() << "\n";
		}

		GlobalLogger().info << "Cleaned " << chunksCleaned << " chunks across " << regionCoords.size() << " regions.\n";

		// Delete original, copy from temp, delete temp
		regionManager.Release();
		outRegionManager.Release();
		fs::remove_all(_relPath + "/region");
		fs::copy(_relPath + "/tempRegion", _relPath + "/region");
		fs::remove_all(_relPath + "/tempRegion");
	}

	// Nether
	{
		if (!regionManager.Initialize(_relPath + "/DIM-1/region")) {
			GlobalLogger().error << "Failed to initialize nether region manager for cleaning! (Skipping)\n";
		}

		if (!recreateTempDir(_relPath + "/tempRegionNether")) {
			GlobalLogger().error << "Failed to recreate temp directory!\n";
			return false;
		}

		if (!outRegionManager.Initialize(_relPath + "/tempRegionNether")) {
			GlobalLogger().error << "Failed to initialize output region manager for cleaning!\n";
			return false;
		}

		// Check what regions exist
		GlobalLogger().info << "Scanning for regions to clean...\n";
		std::vector<Int32_2> regionCoords;
		for (const auto& entry : fs::directory_iterator(_relPath + "/DIM-1/region")) {
			const fs::path& regionPath = entry.path();
			// Is this a valid region file?
			if (regionPath.extension() == ".mcr") {
				// Get the coordinate pair for this region
				const std::string filename = entry.path().filename().string();
				int rx, rz;

				// Does this have a valid name?
				if (std::sscanf(filename.c_str(), "r.%d.%d.mcr", &rx, &rz) == 2) {
					regionCoords.push_back({ rx, rz });
				}
			}
		}

		GlobalLogger().info << "Found " << regionCoords.size() << " regions to clean.\n";

		// Go through each region
		int chunksCleaned = 0;
		int regionCnt = 0;
		for (auto& regionCoord : regionCoords) {
			auto region = regionManager.LoadRegion(regionCoord);
			outRegionManager.CreateRegion(regionCoord);
			auto newRegion = outRegionManager.LoadRegion(regionCoord);
			for (int cx = 0; cx < 32; cx++) {
				for (int cz = 0; cz < 32; cz++) {
					// Load the chunk if it exists
					if (region->ChunkExists({ cx, cz })) {
						chunksCleaned++;
						auto chunk = region->GetChunk({ regionCoord.x * 32 + cx, regionCoord.z * 32 + cz });
						// Save to a new region file
						newRegion->AddChunk(chunk, 0, {});
					}
				}
			}
			regionCnt++;
			GlobalLogger().info << "Cleaned region (Nether):" << regionCoord << ": " << regionCnt << " / "
			                    << regionCoords.size() << "\n";
		}

		GlobalLogger().info << "Cleaned " << chunksCleaned << " chunks across " << regionCoords.size() << " regions.\n";

		// Delete original, copy from temp, delete temp
		regionManager.Release();
		outRegionManager.Release();
		fs::remove_all(_relPath + "/DIM-1/region");
		fs::copy(_relPath + "/tempRegionNether", _relPath + "/DIM-1/region");
		fs::remove_all(_relPath + "/tempRegionNether");
	}
	return true; // yay we did it
}
}; // namespace Utilities