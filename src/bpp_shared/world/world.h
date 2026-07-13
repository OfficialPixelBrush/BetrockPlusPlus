/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

// The world manager acts like a wrapper around the chunk manager and lighting manager.
// It handles all world-related operations and provides a simple interface for the rest of the code to interact with the world.
// WorldManager.h
#pragma once
#include "BS_thread_pool.hpp"
#include "base_structs.h"
#include "blocks.h"
#include "entities/entity_manager.h"
#include "helpers/AABB.h"
#include "java_math.h"
#include "lighter.h"
#include "tile_entities/tile_entity_manager.h"
#include "world/chunk.h"
#include "world/client_pos.h"
#include "world/storage/region_manager.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

struct PendingBlock {
	Block block{ BLOCK_AIR, 0 };
	Int3 block_pos{ 0, 0, 0 };
	Int2 light{ 0, 15 }; // block light, sky light
};

struct WorldManager {
	RegionManager* regionManager = nullptr;
	std::unordered_map<Int32_2, std::shared_ptr<Chunk>> chunks;
	std::function<void(PendingBlock, Int32_2)> onBlockUpdate;

	std::unordered_map<Int32_2, std::vector<std::pair<Int3, Block>>> pendingBleedWrites;

	std::mutex genDoneMutex;
	std::deque<std::shared_ptr<Chunk>> genDoneQueue;

	Lighter lightManager;
	TileEntityManager tileEntityManager;
	EntityManager entityManager;

	BS::thread_pool<> pool{ 2 };
	BS::thread_pool<> populationPool{ 1 }; // unused

	int64_t seed = 0;
	TickTime elapsed_ticks = 0;

	Int3 spawnPoint{ 0, 0, 0 };

	Java::Random rand;

	WorldManager(bool pIsHell = false) : isHell(pIsHell) {
		entityManager.world = this; // Bind the world pointer in EntityManager
	}

	~WorldManager() {}

	void initWorldSeed(std::string pSeed) {
		this->seed = hashCode(pSeed);
	}

	void initWorldSeed(int64_t pSeed) {
		this->seed = pSeed;
	}

	void tick(const std::vector<ClientPosition>& players);
	void update(const std::vector<ClientPosition>& players);
	void shutdown();
	std::vector<AABB> getCollidingBoundingBoxes(const AABB& area);
	int getViewRadius() {
		return VIEW_RADIUS;
	}
	int getSimulationDistance() {
		return SIMULATION_RADIUS;
	}
	void updateLoadRadius(const std::vector<ClientPosition>& players);
	void pumpPipeline(const std::vector<ClientPosition>& players);
	void populateReady();
	void drainLoadQueue();

	void notifyNeighborsOfUpdate(Int3 globalPos) {
		// Update our six neighbors
		const int ndx[] = { -1, 1, 0, 0 };
		const int ndz[] = { 0, 0, -1, 1 };
		
		// Notify horizontal neighbors
		for (int i = 0; i < 4; i++) {
			auto dx = ndx[i];
			auto dz = ndz[i];
			Int3 newPos = { globalPos.x + dx, globalPos.y, globalPos.z + dz };
			auto block = this->getBlockId(newPos);
			auto updateFunction = Blocks::blockBehaviors[block].onNeighborBlockChange;
			if (updateFunction)
				updateFunction(*this, newPos);
		}

		// Vertical neighbors
		for (int i = 0; i < 2; i++) {
			auto dy = ndx[i]; // we are using ndx because the first two items are -1, 1
			Int3 newPos = { globalPos.x, globalPos.y + dy, globalPos.z };
			auto block = this->getBlockId(newPos);
			auto updateFunction = Blocks::blockBehaviors[block].onNeighborBlockChange;
			if (updateFunction)
				updateFunction(*this, newPos);
		}
	}

	// For creating a fresh tile entity for generation etc
	void createTileEntity(std::shared_ptr<TileEntity> tileEntity) {
		Int32_2 cpos{ tileEntity->m_position.x >> 4, tileEntity->m_position.z >> 4 };
		Chunk* chunk = getChunkRaw(cpos);
		if (!chunk)
			return;
		tileEntity->m_chunk = chunk;
		tileEntityManager.initializeTileEntity(tileEntity);   // weak_ptr added if canTick
		chunk->tileEntities.push_back(std::move(tileEntity)); // chunk takes ownership
	}

	// For registering a tile entity that already exists in the world (e.g. loaded from disk)
	void registerChunkTileEntities(Chunk* chunk) {
		for (auto& te : chunk->tileEntities) {
			if (te) {
				tileEntityManager.initializeTileEntity(te);
				te->m_chunk = chunk;
			}
		}
	}

	// Returns the tile entity at world m_position `pos`, or nullptr if none.
	TileEntity* getTileEntity(Int3 pos) {
		Chunk* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
		if (!chunk)
			return nullptr;
		for (auto& te : chunk->tileEntities) {
			if (te && te->m_position.x == pos.x && te->m_position.y == pos.y && te->m_position.z == pos.z)
				return te.get();
		}
		return nullptr;
	}

	// Returns nullptr if not found or wrong type.
	template <typename T>
	T* getTileEntityAs(Int3 pos) {
		return dynamic_cast<T*>(getTileEntity(pos));
	}

	template <typename T>
	std::shared_ptr<T> getTileEntityShared(Int3 pos) {
		Chunk* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
		if (!chunk)
			return nullptr;
		for (auto& te : chunk->tileEntities) {
			if (te && te->m_position.x == pos.x && te->m_position.y == pos.y && te->m_position.z == pos.z)
				return std::dynamic_pointer_cast<T>(te);
		}
		return nullptr;
	}

	// Remove the tile entity at world m_position `pos`.
	void removeTileEntity(Int3 pos) {
		Chunk* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
		if (!chunk)
			return;
		auto& tes = chunk->tileEntities;
		tes.erase(std::remove_if(tes.begin(), tes.end(),
		                         [&](const std::shared_ptr<TileEntity>& te) {
			                         return te && te->m_position.x == pos.x && te->m_position.y == pos.y &&
			                                te->m_position.z == pos.z;
		                         }),
		          tes.end());
	}

	// Called from pool gen threads
	void postGenResult(std::shared_ptr<Chunk> chunk) {
		std::lock_guard lk(genDoneMutex);
		genDoneQueue.push_back(std::move(chunk));
	}

	void drainGenQueue();

	std::shared_ptr<Chunk> getChunk(Int32_2 pos) {
		return getChunkShared(pos);
	}

	bool canPopulate(Int32_2 pos) {
		return canPopulateDirect(pos);
	}

	BlockType getBlockId(Int3 wpos) {
		if (!inBounds(wpos.y))
			return BlockType::BLOCK_AIR;
		auto* chunk = getChunkRaw({ wpos.x >> 4, wpos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return BlockType::BLOCK_AIR;
		return chunk->getBlock({ wpos.x & 15, wpos.y, wpos.z & 15 });
	}

	uint8_t getMetadata(Int3 wpos) {
		if (!inBounds(wpos.y))
			return 0;
		auto* chunk = getChunkRaw({ wpos.x >> 4, wpos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->getMeta({ wpos.x & 15, wpos.y, wpos.z & 15 });
	}

	void setBlock(Int3 wpos, const Block& block) {
		setBlock(wpos, block.type, block.data);
	}

	void setMeta(Int3 wpos, uint8_t metadata = 0);

	void setBlock(Int3 wpos, BlockType block_type, uint8_t metadata = 0);

	bool isAirBlock(Int3 wpos) {
		return getBlockId(wpos) == BlockType::BLOCK_AIR;
	}

	Material getMaterial(Int3 wpos) {
		return Blocks::blockProperties[getBlockId(wpos)].material;
	}

	bool isBlockNormalCube(Int3 wpos) {
		BlockType block = getBlockId(wpos);
		if (block == BlockType::BLOCK_AIR)
			return false;
		const auto& props = Blocks::blockProperties[block];
		return props.material.isSolid && props.isNormalCube;
	}

	int findTopSolidBlock(int wx, int wz);

	void initSpawn();

	// Force generate a chunk synchronously, blocking until the chunk is fully generated
	void forceGenChunkSync(Int32_2 pos);

	Int3 getSpawnPoint(bool adjust) {
		if (!adjust)
			return spawnPoint;
		int sx = this->spawnPoint.x;
		int sz = this->spawnPoint.z;
		sx += rand.nextInt(20) - 10;
		sz += rand.nextInt(20) - 10;
		int sy = findTopSolidBlock(sx, sz);
		return { sx, sy, sz };
	}

	BlockType getFirstUncoveredBlock(int wx, int wz);

	int getHeightValue(int wx, int wz) {
		auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->getHeightValue({ wx & 15, wz & 15 });
	}

	// Returns the baked temperature/humidity for a world column.
	double getTemperatureAt(int wx, int wz) {
		auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->getTemperature({ wx & 15, wz & 15 }));
	}

	double getHumidityAt(int wx, int wz) {
		auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->getHumidity({ wx & 15, wz & 15 }));
	}

	int getSkyLight(Int3 pos) {
		if (!inBounds(pos.y))
			return 0;
		auto* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->getSkyLight({ pos.x & 15, pos.y, pos.z & 15 });
	}

	int getBlockLight(Int3 pos) {
		if (!inBounds(pos.y))
			return 0;
		auto* chunk = getChunkRaw({ pos.x >> 4, pos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->getBlockLight({ pos.x & 15, pos.y, pos.z & 15 });
	}

	void propagateChunkLightBorders(Int32_2 cpos);

	Chunk* getChunkRaw(Int32_2 pos) {
		auto it = chunks.find(pos);
		return (it != chunks.end()) ? it->second.get() : nullptr;
	}

	bool isChunkValid(Int32_2 pos) {
		auto* chunk = getChunkRaw({ pos.x, pos.z });
		if (!chunk)
			return false;
		if (chunk->state.load() >= ChunkState::Generated)
			return true;
		return false;
	}

	Int32_2 blockToChunkPos(Int32_2 blockPos) {
		return { blockPos.x >> 4, blockPos.z >> 4 };
	}

	void flushBleedWrites();

	// Returns true when the world-space Y is within valid chunk bounds.
	static constexpr bool inBounds(int y) {
		return y >= 0 && y < CHUNK_HEIGHT;
	}

private:
	// I believe the vanilla default is
	static constexpr int VIEW_RADIUS = 12;
	static constexpr int SIMULATION_RADIUS = 9;

	bool isHell = false; // for the nether

	void seedChunkLighting(Int32_2 pos);

	std::shared_ptr<Chunk> getChunkShared(Int32_2 pos) {
		auto it = chunks.find(pos);
		return (it != chunks.end()) ? it->second : nullptr;
	}

	// Check if a chunk can be populated
	bool canPopulateDirect(Int32_2 pos) {
		auto* chunk = getChunkRaw(pos);
		if (!chunk)
			return false;
		if (chunk->isTerrainPopulated)
			return false;
		if (chunk->state.load() < ChunkState::Generated)
			return false;
		auto* a = getChunkRaw({ pos.x + 1, pos.z });
		auto* b = getChunkRaw({ pos.x, pos.z + 1 });
		auto* c = getChunkRaw({ pos.x + 1, pos.z + 1 });
		if (!a || !b || !c)
			return false;
		if (a->state.load() < ChunkState::Generated)
			return false;
		if (b->state.load() < ChunkState::Generated)
			return false;
		if (c->state.load() < ChunkState::Generated)
			return false;
		return true;
	}
};