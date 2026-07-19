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
#include "chunk.h"
#include "client_pos.h"
#include "entities/entity_manager.h"
#include "helpers/AABB.h"
#include "java_math.h"
#include "lighter.h"
#include "tick_scheduler.h"
#include "tile_entities/tile_entity_manager.h"
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
	Block m_block{ BLOCK_AIR, 0 };
	Int3 m_block_pos{ 0, 0, 0 };
	Int2 m_light{ 0, 15 }; // block light, sky light
};

struct WorldManager {
	std::unordered_map<Int32_2, std::shared_ptr<Chunk>> m_chunks;
	std::function<void(PendingBlock, Int32_2)> m_onBlockUpdate;
	std::unordered_map<Int32_2, std::vector<std::pair<Int3, Block>>> m_pendingBleedWrites;
	std::mutex m_genDoneMutex;
	std::deque<std::shared_ptr<Chunk>> m_genDoneQueue;
	Lighter m_lightManager;
	TileEntityManager m_tileEntityManager;
	EntityManager m_entityManager;
	TickScheduler m_tickScheduler;
	RegionManager* m_regionManager = nullptr;
	int64_t m_seed = 0;
	TickTime m_elapsed_ticks = 0;
	Int3 m_spawnPoint{ 0, 0, 0 };
	Dimension m_thisDimension = Dimension::Overworld;
	BS::thread_pool<> m_pool{ 2 };
	BS::thread_pool<> m_populationPool{ 1 }; // unused
	Java::Random m_rand;

	WorldManager(bool pIsHell = false) : isHell(pIsHell) {
		m_entityManager.m_world = this; // Bind the world pointer in EntityManager
		m_tickScheduler.m_world = this;
		if (isHell)
			m_thisDimension = Dimension::Nether;
	}

	~WorldManager() {}

	void tick(const std::vector<ClientPosition>& players);
	void update(const std::vector<ClientPosition>& players);
	void shutdown();
	std::vector<AABB> getCollidingBoundingBoxes(const AABB& area);
	void flushBleedWrites();
	void propagateChunkLightBorders(Int32_2 cpos);
	BlockType getFirstUncoveredBlock(int wx, int wz);
	int findTopSolidBlock(int wx, int wz);
	void setMeta(Int3 wpos, uint8_t metadata = 0);
	void setBlock(Int3 wpos, BlockType block_type, uint8_t metadata = 0);
	void drainGenQueue();
	bool isLiquidInAABB(AABB collider);
	void initSpawn();
	bool handleFluidAcceleration(AABB collider, Material material, Entity& entity);
	bool isMaterialInAABB(AABB collider, Material material);
	void updateLoadRadius(const std::vector<ClientPosition>& players);
	void pumpPipeline(const std::vector<ClientPosition>& players);
	void populateReady();
	void drainLoadQueue();

	int getViewRadius() {
		return VIEW_RADIUS;
	}
	int getSimulationDistance() {
		return SIMULATION_RADIUS;
	}

	void initWorldSeed(std::string pSeed) {
		this->m_seed = hashCode(pSeed);
	}

	void initWorldSeed(int64_t pSeed) {
		this->m_seed = pSeed;
	}

	void notifyNeighborsOfUpdate(Int3 globalPos) {
		// Update our six neighbors
		const int ndx[] = { -1, 1, 0, 0 };
		const int ndz[] = { 0, 0, -1, 1 };

		// Notify horizontal neighbors
		for (int i = 0; i < 4; i++) {
			auto dx = ndx[i];
			auto dz = ndz[i];
			Int3 newPos = { globalPos.m_x + dx, globalPos.m_y, globalPos.m_z + dz };
			auto block = this->getBlockId(newPos);
			auto updateFunction = Blocks::blockBehaviors[block].m_onNeighborBlockChange;
			if (updateFunction)
				updateFunction(*this, newPos);
		}

		// Vertical neighbors
		for (int i = 0; i < 2; i++) {
			auto dy = ndx[i]; // we are using ndx because the first two items are -1, 1
			Int3 newPos = { globalPos.m_x, globalPos.m_y + dy, globalPos.m_z };
			auto block = this->getBlockId(newPos);
			auto updateFunction = Blocks::blockBehaviors[block].m_onNeighborBlockChange;
			if (updateFunction)
				updateFunction(*this, newPos);
		}
	}

	// For creating a fresh tile entity for generation etc
	void createTileEntity(std::shared_ptr<TileEntity> tileEntity) {
		Int32_2 cpos{ tileEntity->m_position.m_x >> 4, tileEntity->m_position.m_z >> 4 };
		Chunk* chunk = getChunkRaw(cpos);
		if (!chunk)
			return;
		tileEntity->m_chunk = chunk;
		m_tileEntityManager.initializeTileEntity(tileEntity);   // weak_ptr added if canTick
		chunk->m_tileEntities.push_back(std::move(tileEntity)); // chunk takes ownership
	}

	// For registering a tile entity that already exists in the world (e.g. loaded from disk)
	void registerChunkTileEntities(Chunk* chunk) {
		for (auto& te : chunk->m_tileEntities) {
			if (te) {
				m_tileEntityManager.initializeTileEntity(te);
				te->m_chunk = chunk;
			}
		}
	}

	// Returns the tile entity at world m_position `pos`, or nullptr if none.
	TileEntity* getTileEntity(Int3 pos) {
		Chunk* chunk = getChunkRaw({ pos.m_x >> 4, pos.m_z >> 4 });
		if (!chunk)
			return nullptr;
		for (auto& te : chunk->m_tileEntities) {
			if (te && te->m_position.m_x == pos.m_x && te->m_position.m_y == pos.m_y && te->m_position.m_z == pos.m_z)
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
		Chunk* chunk = getChunkRaw({ pos.m_x >> 4, pos.m_z >> 4 });
		if (!chunk)
			return nullptr;
		for (auto& te : chunk->m_tileEntities) {
			if (te && te->m_position.m_x == pos.m_x && te->m_position.m_y == pos.m_y && te->m_position.m_z == pos.m_z)
				return std::dynamic_pointer_cast<T>(te);
		}
		return nullptr;
	}

	// Remove the tile entity at world m_position `pos`.
	void removeTileEntity(Int3 pos) {
		Chunk* chunk = getChunkRaw({ pos.m_x >> 4, pos.m_z >> 4 });
		if (!chunk)
			return;
		auto& tes = chunk->m_tileEntities;
		tes.erase(std::remove_if(tes.begin(), tes.end(),
		                         [&](const std::shared_ptr<TileEntity>& te) {
			                         return te && te->m_position.m_x == pos.m_x && te->m_position.m_y == pos.m_y &&
			                                te->m_position.m_z == pos.m_z;
		                         }),
		          tes.end());
	}

	// Called from pool gen threads
	void postGenResult(std::shared_ptr<Chunk> chunk) {
		std::lock_guard lk(m_genDoneMutex);
		m_genDoneQueue.push_back(std::move(chunk));
	}

	std::shared_ptr<Chunk> getChunk(Int32_2 pos) {
		return getChunkShared(pos);
	}

	bool canPopulate(Int32_2 pos) {
		return canPopulateDirect(pos);
	}

	BlockType getBlockId(Int3 wpos) {
		if (!inBounds(wpos.m_y))
			return BlockType::BLOCK_AIR;
		auto* chunk = getChunkRaw({ wpos.m_x >> 4, wpos.m_z >> 4 });
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return BlockType::BLOCK_AIR;
		return chunk->getBlock({ wpos.m_x & 15, wpos.m_y, wpos.m_z & 15 });
	}

	uint8_t getMetadata(Int3 wpos) {
		if (!inBounds(wpos.m_y))
			return 0;
		auto* chunk = getChunkRaw({ wpos.m_x >> 4, wpos.m_z >> 4 });
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return 0;
		return chunk->getMeta({ wpos.m_x & 15, wpos.m_y, wpos.m_z & 15 });
	}

	void setBlock(Int3 wpos, const Block& block) {
		setBlock(wpos, block.m_type, block.m_data);
	}

	bool isAirBlock(Int3 wpos) {
		return getBlockId(wpos) == BlockType::BLOCK_AIR;
	}

	Material getMaterial(Int3 wpos) {
		return Blocks::blockProperties[getBlockId(wpos)].m_material;
	}

	bool isBlockNormalCube(Int3 wpos) {
		BlockType block = getBlockId(wpos);
		if (block == BlockType::BLOCK_AIR)
			return false;
		const auto& props = Blocks::blockProperties[block];
		return props.m_material.m_isSolid && props.m_isNormalCube;
	}

	Int3 getSpawnPoint(bool adjust) {
		if (!adjust)
			return m_spawnPoint;
		int sx = this->m_spawnPoint.m_x;
		int sz = this->m_spawnPoint.m_z;
		sx += m_rand.nextInt(20) - 10;
		sz += m_rand.nextInt(20) - 10;
		int sy = findTopSolidBlock(sx, sz);
		return { sx, sy, sz };
	}

	int getHeightValue(int wx, int wz) {
		auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return 0;
		return chunk->getHeightValue({ wx & 15, wz & 15 });
	}

	// Returns the baked temperature/humidity for a world column.
	double getTemperatureAt(int wx, int wz) {
		auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->getTemperature({ wx & 15, wz & 15 }));
	}

	double getHumidityAt(int wx, int wz) {
		auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->getHumidity({ wx & 15, wz & 15 }));
	}

	int getSkyLight(Int3 pos) {
		if (!inBounds(pos.m_y))
			return 0;
		auto* chunk = getChunkRaw({ pos.m_x >> 4, pos.m_z >> 4 });
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return 0;
		return chunk->getSkyLight({ pos.m_x & 15, pos.m_y, pos.m_z & 15 });
	}

	int getBlockLight(Int3 pos) {
		if (!inBounds(pos.m_y))
			return 0;
		auto* chunk = getChunkRaw({ pos.m_x >> 4, pos.m_z >> 4 });
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return 0;
		return chunk->getBlockLight({ pos.m_x & 15, pos.m_y, pos.m_z & 15 });
	}

	Chunk* getChunkRaw(Int32_2 pos) {
		auto it = m_chunks.find(pos);
		return (it != m_chunks.end()) ? it->second.get() : nullptr;
	}

	bool isChunkValid(Int32_2 pos) {
		auto* chunk = getChunkRaw({ pos.m_x, pos.m_z });
		if (!chunk)
			return false;
		if (chunk->m_state.load() >= ChunkState::Generated)
			return true;
		return false;
	}

	bool AABBinValidChunks(AABB collider) {
		if (collider.m_minY < 0.0 || collider.m_maxY >= 128.0)
			return false;
		int minCX = MathHelper::floor_double(collider.m_minX) >> 4;
		int maxCX = MathHelper::floor_double(collider.m_maxX + 1.0) >> 4;
		int minCZ = MathHelper::floor_double(collider.m_minZ) >> 4;
		int maxCZ = MathHelper::floor_double(collider.m_maxZ + 1.0) >> 4;

		for (int cx = minCX; cx <= maxCX; cx++) {
			for (int cz = minCZ; cz <= maxCZ; cz++) {
				if (!isChunkValid({ cx, cz }))
					return false;
			}
		}
		return true;
	}

	Int32_2 blockToChunkPos(Int32_2 blockPos) {
		return { blockPos.m_x >> 4, blockPos.m_z >> 4 };
	}

	// Returns true when the world-space Y is within valid chunk bounds.
	static constexpr bool inBounds(int y) {
		return y >= 0 && y < CHUNK_HEIGHT;
	}

private:
	// I believe the vanilla default is
	static constexpr int VIEW_RADIUS = 9;
	static constexpr int SIMULATION_RADIUS = 9;

	bool isHell = false; // for the nether

	void seedChunkLighting(Int32_2 pos);

	std::shared_ptr<Chunk> getChunkShared(Int32_2 pos) {
		auto it = m_chunks.find(pos);
		return (it != m_chunks.end()) ? it->second : nullptr;
	}

	// Check if a chunk can be populated
	bool canPopulateDirect(Int32_2 pos) {
		auto* chunk = getChunkRaw(pos);
		if (!chunk)
			return false;
		if (chunk->m_isTerrainPopulated)
			return false;
		if (chunk->m_state.load() < ChunkState::Generated)
			return false;
		auto* a = getChunkRaw({ pos.m_x + 1, pos.m_z });
		auto* b = getChunkRaw({ pos.m_x, pos.m_z + 1 });
		auto* c = getChunkRaw({ pos.m_x + 1, pos.m_z + 1 });
		if (!a || !b || !c)
			return false;
		if (a->m_state.load() < ChunkState::Generated)
			return false;
		if (b->m_state.load() < ChunkState::Generated)
			return false;
		if (c->m_state.load() < ChunkState::Generated)
			return false;
		return true;
	}
};