/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "chunk.h"
#include "entities/entity.h"
#include "logger.h"
#include "player_conn/player_session.h"
#include "storage/region.h"
#include "storage/region_manager.h"

void PrintStructSizes() {
	GlobalLogger().debug << "Chunk: " << sizeof(Chunk) << " Bytes\n";
	GlobalLogger().debug << "RegionManager: " << sizeof(RegionManager) << " Bytes\n";
	GlobalLogger().debug << "Region: " << sizeof(Region) << " Bytes\n";
	GlobalLogger().debug << "Entity: " << sizeof(Entity) << " Bytes\n";
	GlobalLogger().debug << "PlayerSession: " << sizeof(PlayerSession) << " Bytes\n";
}