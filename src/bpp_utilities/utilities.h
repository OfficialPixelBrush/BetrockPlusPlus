/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include <filesystem>
#include <iostream>
#include <cstdio>
#include "world/storage/region_manager.h"
#include "world/storage/save_manager.h"

namespace fs = std::filesystem;

namespace Utilities {
	// Creates a temp directory and deletes it if it already exists. Returns false on failure.
	inline bool recreateTempDir(const fs::path& dir) {
		std::error_code ec;
		fs::remove_all(dir, ec);
		if (ec) {
			GlobalLogger().error << "Failed to remove directory: " << ec.message() << '\n';
			return false;
		}
		fs::create_directories(dir, ec);
		if (ec) {
			GlobalLogger().error << "Failed to create directory: " << ec.message() << '\n';
			return false;
		}
		return true;
	}

	// Cleans up level data since mcr files can become bloated
	inline bool cleanLevel(std::string relPath) {
		SaveManager saveManager;
		RegionManager regionManager;
		RegionManager outRegionManager;

		if (!saveManager.initialize(relPath)) {
			GlobalLogger().error << "Failed to initialize save manager for cleaning!\n";
			return false;
		}

		// Overworld first
		{
			if (!regionManager.initialize(relPath + "/region")) {
				GlobalLogger().error << "Failed to initialize overworld region manager for cleaning! (Skipping)\n";
			}

			if (!recreateTempDir(relPath + "/tempRegion")) {
				GlobalLogger().error << "Failed to recreate temp directory!\n";
				return false;
			}

			if (!outRegionManager.initialize(relPath + "/tempRegion")) {
				GlobalLogger().error << "Failed to initialize output region manager for cleaning!\n";
				return false;
			}

			// Check what regions exist
			GlobalLogger().info << "Scanning for regions to clean...\n";
			std::vector<Int32_2> regionCoords;
			for (const auto& entry : fs::directory_iterator(relPath + "/region")) {
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
				auto region = regionManager.loadRegion(regionCoord);
				outRegionManager.createRegion(regionCoord);
				auto newRegion = outRegionManager.loadRegion(regionCoord);
				for (int cx = 0; cx < 32; cx++) {
					for (int cz = 0; cz < 32; cz++) {
						// Load the chunk if it exists
						if (region->chunkExists({ cx, cz })) {
							chunksCleaned++;
							auto chunk = region->GetChunk({ regionCoord.x * 32 + cx, regionCoord.z * 32 + cz });
							// Save to a new region file
							newRegion->AddChunk(chunk);
						}
					}
				}
				regionCnt++;
				GlobalLogger().info << "Cleaned region (Overworld):" << regionCoord << ": " << regionCnt << " / " << regionCoords.size() << "\n";
			}

			GlobalLogger().info << "Cleaned " << chunksCleaned << " chunks across " << regionCoords.size() << " regions.\n";

			// Delete original, copy from temp, delete temp
			regionManager.release();
			outRegionManager.release();
			fs::remove_all(relPath + "/region");
			fs::copy(relPath + "/tempRegion", relPath + "/region");
			fs::remove_all(relPath + "/tempRegion");
		}

		// Nether
		{
			if (!regionManager.initialize(relPath + "/DIM-1/region")) {
				GlobalLogger().error << "Failed to initialize nether region manager for cleaning! (Skipping)\n";
			}

			if (!recreateTempDir(relPath + "/tempRegionNether")) {
				GlobalLogger().error << "Failed to recreate temp directory!\n";
				return false;
			}

			if (!outRegionManager.initialize(relPath + "/tempRegionNether")) {
				GlobalLogger().error << "Failed to initialize output region manager for cleaning!\n";
				return false;
			}

			// Check what regions exist
			GlobalLogger().info << "Scanning for regions to clean...\n";
			std::vector<Int32_2> regionCoords;
			for (const auto& entry : fs::directory_iterator(relPath + "/DIM-1/region")) {
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
				auto region = regionManager.loadRegion(regionCoord);
				outRegionManager.createRegion(regionCoord);
				auto newRegion = outRegionManager.loadRegion(regionCoord);
				for (int cx = 0; cx < 32; cx++) {
					for (int cz = 0; cz < 32; cz++) {
						// Load the chunk if it exists
						if (region->chunkExists({ cx, cz })) {
							chunksCleaned++;
							auto chunk = region->GetChunk({ regionCoord.x * 32 + cx, regionCoord.z * 32 + cz });
							// Save to a new region file
							newRegion->AddChunk(chunk);
						}
					}
				}
				regionCnt++;
				GlobalLogger().info << "Cleaned region (Nether):" << regionCoord << ": " << regionCnt << " / " << regionCoords.size() << "\n";
			}

			GlobalLogger().info << "Cleaned " << chunksCleaned << " chunks across " << regionCoords.size() << " regions.\n";

			// Delete original, copy from temp, delete temp
			regionManager.release();
			outRegionManager.release();
			fs::remove_all(relPath + "/DIM-1/region");
			fs::copy(relPath + "/tempRegionNether", relPath + "/DIM-1/region");
			fs::remove_all(relPath + "/tempRegionNether");
		}
		return true; // yay we did it
	}
};