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
#include "world/generator/nether/chunk_gen.h"
#include "world/generator/overworld/chunk_gen.h"

void PrintStructSizes() {
	GlobalLogger().debug << "Chunk: " << sizeof(Chunk) << " Bytes (includes " << SUB_CHUNK_COUNT << " x "
	                     << sizeof(SubChunk) << " Byte sub-chunk headers)\n";
	GlobalLogger().debug << "SubChunk layers, allocated independently: Blocks " << sizeof(SubChunk::BlockLayer)
	                     << " Bytes, Meta/BlockLight/SkyLight " << sizeof(SubChunk::NibbleLayer) << " Bytes each\n";
	GlobalLogger().debug << "RegionManager: " << sizeof(RegionManager) << " Bytes\n";
	GlobalLogger().debug << "Region: " << sizeof(Region) << " Bytes\n";
	GlobalLogger().debug << "Entity: " << sizeof(Entity) << " Bytes\n";
	GlobalLogger().debug << "PlayerSession: " << sizeof(PlayerSession) << " Bytes\n";
	GlobalLogger().debug << "OverworldGenerator: " << sizeof(OverworldGenerator) << " Bytes\n";
	GlobalLogger().debug << "NetherGenerator: " << sizeof(NetherGenerator) << " Bytes\n";
}