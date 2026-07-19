/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "crafting/recipe_manager.h"
#include "logger.h"
#include "world/storage/region_manager.h"
#include "world/storage/save_manager.h"
#include "world/world.h"

// General game runtime that the client and server can use so that way we don't reuse a bunch of code and have to maintain it in two places.
struct Runtime {
	// Storage
	SaveManager m_saveManager;
	WorldManager m_world;
	WorldManager m_worldHell;
	RegionManager m_overworldRegionManager;
	RegionManager m_hellRegionManager; // hehe i call it hell instead of nether cause im quirky

	// Gameplay
	RecipeManager m_recipeManager;

	Runtime() : m_worldHell(true) {
		Blocks::registerAll();
		Items::registerAll();
		m_recipeManager.addVanillaRecipes();
		GlobalLogger().m_info << "New game runtime created!\n";
	}

	void init(std::string levelPath, std::string seedOverride = "") {
		// Setup our save
		bool newSave = false;
		if (!m_saveManager.initialize(levelPath)) {
			GlobalLogger().m_warn << "**** FAILED TO LOAD WORLD DATA! Attempting to create new world... \n";
			newSave = true;
			if (!m_saveManager.createNewWorld({ .m_RandomSeed = (seedOverride != "")
			                                                    ? m_saveManager.seedFromString(seedOverride)
			                                                    : Java::Random().nextLong() })) {
				GlobalLogger().m_error << "**** FAILED TO CREATE NEW WORLD! \n";
				exit(1);
			}
			GlobalLogger().m_info << "New world created successfully. \n";
		}

		// Initialize our region managers
		m_overworldRegionManager.initialize(levelPath + "/region");
		m_hellRegionManager.initialize(levelPath + "/DIM-1/region");

		// Bind our pointers
		m_overworldRegionManager.m_world = &m_world;
		m_hellRegionManager.m_world = &m_worldHell;

		// Initialize save data with our world objects
		m_saveManager.loadLevelData();
		m_world.initWorldSeed(m_saveManager.getLevelData().m_RandomSeed);
		m_worldHell.initWorldSeed(m_saveManager.getLevelData().m_RandomSeed);

		// World time
		m_world.m_elapsed_ticks = m_saveManager.getLevelData().m_time;
		m_worldHell.m_elapsed_ticks = m_saveManager.getLevelData().m_time;

		// Bind the region managers with the world objects
		m_world.m_regionManager = &m_overworldRegionManager;
		m_worldHell.m_regionManager = &m_hellRegionManager;

		// If we created a new save then make a new spawn point
		if (newSave) {
			m_world.initSpawn();
		} else {
			m_world.m_spawnPoint = m_saveManager.getLevelData().m_spawnPoint;
		}
		m_worldHell.m_spawnPoint = m_world.m_spawnPoint; // Interestingly the world spawn doesn't have the /= or *= 8 stuff

		GlobalLogger().m_info << "Game runtime initialized!\n";
	}
};