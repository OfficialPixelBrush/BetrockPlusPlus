/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "cross_platform.h"
#include "logger.h"
#include "world/client_pos.h"
#include "world/storage/region_manager.h"
#include "world/storage/save_manager.h"
#include "world/world.h"
#include <chrono>
#include <memory>
#include <thread>
#include <unordered_set>
#include <vector>

// General game runtime that the client and server can use so that way we don't reuse a bunch of code and have to maintain it in two places.
struct Runtime {
	// Storage
	SaveManager saveManager;
	WorldManager world;
	WorldManager worldHell;
	RegionManager overworldRegionManager;
	RegionManager hellRegionManager; // hehe i call it hell instead of nether cause im quirky

	Runtime() : worldHell(true) {
		Blocks::registerAll();
		GlobalLogger().info << "New game runtime created!\n";
	}

	void init(std::string levelPath, std::string seedOverride = "") {
		// Setup our save
		bool newSave = false;
		if (!saveManager.initialize(levelPath)) {
			GlobalLogger().warn << "**** FAILED TO LOAD WORLD DATA! Attempting to create new world... \n";
			newSave = true;
			if (!saveManager.createNewWorld(
			        { .RandomSeed = (seedOverride != "") ? saveManager.seedFromString(seedOverride) : Java::Random().nextLong() })) {
				GlobalLogger().error << "**** FAILED TO CREATE NEW WORLD! \n";
				exit(1);
			}
			GlobalLogger().info << "New world created successfully. \n";
		}

		// Initialize our region managers
		overworldRegionManager.initialize(levelPath + "/region");
		hellRegionManager.initialize(levelPath + "/DIM-1/region");

		// Initialize save data with our world objects
		saveManager.loadLevelData();
		world.initWorldSeed(saveManager.getLevelData().RandomSeed);
		worldHell.initWorldSeed(saveManager.getLevelData().RandomSeed);

		// World time
		world.elapsed_ticks = saveManager.getLevelData().time;
		worldHell.elapsed_ticks = saveManager.getLevelData().time;

		// Bind the region managers with the world objects
		world.regionManager = &overworldRegionManager;
		worldHell.regionManager = &hellRegionManager;

		// If we created a new save then make a new spawn point
		if (newSave) {
			world.initSpawn();
		} else {
			world.spawnPoint = saveManager.getLevelData().spawnPoint;
		}
		worldHell.spawnPoint = world.spawnPoint; // Interestingly the world spawn doesn't have the /= or *= 8 stuff

		GlobalLogger().info << "Game runtime initialized!\n";
	}
};