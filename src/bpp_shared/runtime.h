/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "crafting/recipe_manager.h"
#include "items/tool_item_properties.h"
#include "logger.h"
#include "world/storage/region_manager.h"
#include "world/storage/save_manager.h"
#include "world/world.h"

// General game runtime that the client and server can use so that way we don't reuse a bunch of code and have to maintain it in two places.
struct Runtime {
	// Storage
	SaveManager saveManager;
	WorldManager world;
	WorldManager worldHell;
	RegionManager overworldRegionManager;
	RegionManager hellRegionManager; // hehe i call it hell instead of nether cause im quirky

	// Gameplay
	RecipeManager recipeManager;

	// Each world shares an entity id counter
	EntityId sharedEntityId = 2;

	Runtime() : worldHell(true) {
		Blocks::RegisterAll();
		Items::RegisterAll();
		recipeManager.AddVanillaRecipes();
		world.entityManager.nextEntityId = &sharedEntityId;
		worldHell.entityManager.nextEntityId = &sharedEntityId;
		GlobalLogger().info << "New game runtime created!\n";
	}

	void Init(std::string _levelPath, std::string _seedOverride = "", int _renderDistance = 8) {
		// Override our view distance
		world.SetViewRadius(_renderDistance);
		worldHell.SetViewRadius(_renderDistance);
		GlobalLogger().info << "Render distance set to " << _renderDistance << " chunks!\n";

		// Setup our save
		bool newSave = false;
		auto initResult = saveManager.Initialize(_levelPath);
		if (initResult != LevelInitFailureReason::SUCCESS) {
			auto tryNewSave = [&]() -> void {
				GlobalLogger().warn << "**** FAILED TO LOAD WORLD DATA! Attempting to create new world... \n";
				newSave = true;
				if (!saveManager.CreateNewWorld({ .randomSeed = (_seedOverride != "")
				                                                    ? saveManager.SeedFromString(_seedOverride)
				                                                    : Java::Random().NextLong() })) {
					GlobalLogger().error << "**** FAILED TO CREATE NEW WORLD! \n";
					exit(1);
				}
				GlobalLogger().info << "New world created successfully. \n";	
			};

			if (initResult == LevelInitFailureReason::ALPHA_FORMATTED) {
				GlobalLogger().info << "MCA Formatted world detected! Attempting to convert...\n";
				saveManager.Release();
				auto conversionResult = Utilities::convertAlphaLevel(_levelPath);

				if (!conversionResult) {
					// We failed to convert
					GlobalLogger().info << "Failed to convert MCA world! Falling back.\n";
					tryNewSave();
				} else {
					// Re-init our level
					if (saveManager.Initialize(_levelPath) != LevelInitFailureReason::SUCCESS) {
						GlobalLogger().error << "Converted level failed to load!";
						tryNewSave();
					}
				}
			} else {
				tryNewSave();
			}
		}

		// Initialize our region managers
		overworldRegionManager.Initialize(_levelPath + "/region");
		hellRegionManager.Initialize(_levelPath + "/DIM-1/region");

		// Bind our pointers
		overworldRegionManager.world = &world;
		hellRegionManager.world = &worldHell;

		// Initialize save data with our world objects
		saveManager.LoadLevelData();
		world.InitWorldSeed(saveManager.GetLevelData().randomSeed);
		worldHell.InitWorldSeed(saveManager.GetLevelData().randomSeed);

		// World time
		world.elapsedTicks = saveManager.GetLevelData().time;
		worldHell.elapsedTicks = saveManager.GetLevelData().time;

		// Bind the region managers with the world objects
		world.regionManager = &overworldRegionManager;
		worldHell.regionManager = &hellRegionManager;

		// If we created a new save then make a new spawn point
		if (newSave) {
			world.InitSpawn();
		} else {
			world.spawnPoint = saveManager.GetLevelData().spawnPoint;
		}
		worldHell.spawnPoint = world.spawnPoint; // Interestingly the world spawn doesn't have the /= or *= 8 stuff

		GlobalLogger().info << "Game runtime initialized!\n";
	}
};