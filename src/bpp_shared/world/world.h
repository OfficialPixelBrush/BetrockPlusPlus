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
	Block block{ BLOCK_AIR, 0 };
	Int3 block_pos{ 0, 0, 0 };
	Int2 light{ 0, 15 }; // block light, sky light
};

struct WorldManager {
	std::unordered_map<Int32_2, std::shared_ptr<Chunk>> chunks;
	std::function<void(PendingBlock, Int32_2)> onBlockUpdate;
	std::unordered_map<Int32_2, std::vector<std::pair<Int3, Block>>> pendingBleedWrites;
	std::mutex genDoneMutex;
	std::deque<std::shared_ptr<Chunk>> genDoneQueue;
	Lighter lightManager;
	TileEntityManager tileEntityManager;
	EntityManager entityManager;
	TickScheduler tickScheduler;
	RegionManager* regionManager = nullptr;
	int64_t seed = 0;
	TickTime elapsed_ticks = 0;
	Int3 spawnPoint{ 0, 0, 0 };
	Dimension thisDimension = Dimension::Overworld;
	BS::thread_pool<> pool{ 2 };
	BS::thread_pool<> populationPool{ 1 }; // unused
	Java::Random rand;

	WorldManager(bool _pIsHell = false) : isHell(_pIsHell) {
		entityManager.world = this; // Bind the world pointer in EntityManager
		tickScheduler.world = this;
		if (isHell)
			thisDimension = Dimension::Nether;
	}

	~WorldManager() {}

	void tick(const std::vector<ClientPosition>& _players);
	void update(const std::vector<ClientPosition>& _players);
	void shutdown();
	std::vector<AABB> getCollidingBoundingBoxes(const AABB& _area);
	void flushBleedWrites();
	void propagateChunkLightBorders(Int32_2 _cpos);
	BlockType getFirstUncoveredBlock(int _wx, int _wz);
	int findTopSolidBlock(int _wx, int _wz);
	void setMeta(Int3 _wpos, uint8_t _metadata = 0);
	void setBlock(Int3 _wpos, BlockType _block_type, uint8_t _metadata = 0);
	void drainGenQueue();
	bool isLiquidInAABB(AABB _collider);
	void initSpawn();
	bool handleFluidAcceleration(AABB _collider, Material _material, Entity& _entity);
	bool isMaterialInAABB(AABB _collider, Material _material);
	void updateLoadRadius(const std::vector<ClientPosition>& _players);
	void pumpPipeline(const std::vector<ClientPosition>& _players);
	void populateReady();
	void drainLoadQueue();

	const int getViewRadius() {
		return VIEW_RADIUS;
	}
	const int getSimulationDistance() {
		return SIMULATION_RADIUS;
	}
	void setViewRadius(int _viewRadius) {
		int newViewRadius = std::max(3, _viewRadius);
		this->VIEW_RADIUS = newViewRadius;
		this->SIMULATION_RADIUS = std::min(9, newViewRadius);
	}

	void initWorldSeed(std::string _pSeed) {
		this->seed = hashCode(_pSeed);
	}

	void initWorldSeed(int64_t _pSeed) {
		this->seed = _pSeed;
	}

	void notifyNeighborsOfUpdate(Int3 _globalPos) {
		// Update our six neighbors
		const int ndx[] = { -1, 1, 0, 0 };
		const int ndz[] = { 0, 0, -1, 1 };

		// Notify horizontal neighbors
		for (int i = 0; i < 4; i++) {
			auto dx = ndx[i];
			auto dz = ndz[i];
			Int3 newPos = { _globalPos.x + dx, _globalPos.y, _globalPos.z + dz };
			auto block = this->getBlockId(newPos);
			auto updateFunction = Blocks::blockBehaviors[block].onNeighborBlockChange;
			if (updateFunction)
				updateFunction(*this, newPos);
		}

		// Vertical neighbors
		for (int i = 0; i < 2; i++) {
			auto dy = ndx[i]; // we are using ndx because the first two items are -1, 1
			Int3 newPos = { _globalPos.x, _globalPos.y + dy, _globalPos.z };
			auto block = this->getBlockId(newPos);
			auto updateFunction = Blocks::blockBehaviors[block].onNeighborBlockChange;
			if (updateFunction)
				updateFunction(*this, newPos);
		}
	}

	// For creating a fresh tile entity for generation etc
	void createTileEntity(std::shared_ptr<TileEntity> _tileEntity) {
		Int32_2 cpos{ _tileEntity->position.x >> 4, _tileEntity->position.z >> 4 };
		Chunk* chunk = getChunkRaw(cpos);
		if (!chunk)
			return;
		_tileEntity->chunk = chunk;
		tileEntityManager.initializeTileEntity(_tileEntity);   // weak_ptr added if canTick
		chunk->tileEntities.push_back(std::move(_tileEntity)); // chunk takes ownership
	}

	// For registering a tile entity that already exists in the world (e.g. loaded from disk)
	void registerChunkTileEntities(Chunk* _chunk) {
		for (auto& te : _chunk->tileEntities) {
			if (te) {
				tileEntityManager.initializeTileEntity(te);
				te->chunk = _chunk;
			}
		}
	}

	// Returns the tile entity at world position `pos`, or nullptr if none.
	TileEntity* getTileEntity(Int3 _pos) {
		Chunk* chunk = getChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
		if (!chunk)
			return nullptr;
		for (auto& te : chunk->tileEntities) {
			if (te && te->position.x == _pos.x && te->position.y == _pos.y && te->position.z == _pos.z)
				return te.get();
		}
		return nullptr;
	}

	// Returns nullptr if not found or wrong type.
	template <typename T>
	T* getTileEntityAs(Int3 _pos) {
		return dynamic_cast<T*>(getTileEntity(_pos));
	}

	template <typename T>
	std::shared_ptr<T> getTileEntityShared(Int3 _pos) {
		Chunk* chunk = getChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
		if (!chunk)
			return nullptr;
		for (auto& te : chunk->tileEntities) {
			if (te && te->position.x == _pos.x && te->position.y == _pos.y && te->position.z == _pos.z)
				return std::dynamic_pointer_cast<T>(te);
		}
		return nullptr;
	}

	// Remove the tile entity at world position `pos`.
	void removeTileEntity(Int3 _pos) {
		Chunk* chunk = getChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
		if (!chunk)
			return;
		auto& tes = chunk->tileEntities;
		tes.erase(std::remove_if(tes.begin(), tes.end(),
		                         [&](const std::shared_ptr<TileEntity>& _te) {
			                         return _te && _te->position.x == _pos.x && _te->position.y == _pos.y &&
			                                _te->position.z == _pos.z;
		                         }),
		          tes.end());
	}

	// Called from pool gen threads
	void postGenResult(std::shared_ptr<Chunk> _chunk) {
		std::lock_guard lk(genDoneMutex);
		genDoneQueue.push_back(std::move(_chunk));
	}

	std::shared_ptr<Chunk> getChunk(Int32_2 _pos) {
		return getChunkShared(_pos);
	}

	bool canPopulate(Int32_2 _pos) {
		return canPopulateDirect(_pos);
	}

	BlockType getBlockId(Int3 _wpos) {
		if (!inBounds(_wpos.y))
			return BlockType::BLOCK_AIR;
		auto* chunk = getChunkRaw({ _wpos.x >> 4, _wpos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return BlockType::BLOCK_AIR;
		return chunk->getBlock({ _wpos.x & 15, _wpos.y, _wpos.z & 15 });
	}

	uint8_t getMetadata(Int3 _wpos) {
		if (!inBounds(_wpos.y))
			return 0;
		auto* chunk = getChunkRaw({ _wpos.x >> 4, _wpos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->getMeta({ _wpos.x & 15, _wpos.y, _wpos.z & 15 });
	}

	void setBlock(Int3 _wpos, const Block& _block) {
		setBlock(_wpos, _block.type, _block.data);
	}

	bool isAirBlock(Int3 _wpos) {
		return getBlockId(_wpos) == BlockType::BLOCK_AIR;
	}

	Material getMaterial(Int3 _wpos) {
		return Blocks::blockProperties[getBlockId(_wpos)].material;
	}

	bool isBlockNormalCube(Int3 _wpos) {
		BlockType block = getBlockId(_wpos);
		if (block == BlockType::BLOCK_AIR)
			return false;
		const auto& props = Blocks::blockProperties[block];
		return props.material.isSolid && props.isNormalCube;
	}

	Int3 getSpawnPoint(bool _adjust) {
		if (!_adjust)
			return spawnPoint;
		int sx = this->spawnPoint.x;
		int sz = this->spawnPoint.z;
		sx += rand.nextInt(20) - 10;
		sz += rand.nextInt(20) - 10;
		int sy = findTopSolidBlock(sx, sz);
		return { sx, sy, sz };
	}

	int getHeightValue(int _wx, int _wz) {
		auto* chunk = getChunkRaw({ _wx >> 4, _wz >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->getHeightValue({ _wx & 15, _wz & 15 });
	}

	// Returns the baked temperature/humidity for a world column.
	double getTemperatureAt(int _wx, int _wz) {
		auto* chunk = getChunkRaw({ _wx >> 4, _wz >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->getTemperature({ _wx & 15, _wz & 15 }));
	}

	double getHumidityAt(int _wx, int _wz) {
		auto* chunk = getChunkRaw({ _wx >> 4, _wz >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->getHumidity({ _wx & 15, _wz & 15 }));
	}

	int getSkyLight(Int3 _pos) {
		if (!inBounds(_pos.y))
			return 0;
		auto* chunk = getChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->getSkyLight({ _pos.x & 15, _pos.y, _pos.z & 15 });
	}

	int getBlockLight(Int3 _pos) {
		if (!inBounds(_pos.y))
			return 0;
		auto* chunk = getChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->getBlockLight({ _pos.x & 15, _pos.y, _pos.z & 15 });
	}

	Chunk* getChunkRaw(Int32_2 _pos) {
		auto it = chunks.find(_pos);
		return (it != chunks.end()) ? it->second.get() : nullptr;
	}

	bool isChunkValid(Int32_2 _pos) {
		auto* chunk = getChunkRaw({ _pos.x, _pos.z });
		if (!chunk)
			return false;
		if (chunk->state.load() >= ChunkState::Generated)
			return true;
		return false;
	}

	bool AABBinValidChunks(AABB _collider) {
		if (_collider.minY < 0.0 || _collider.maxY >= 128.0)
			return false;
		int minCX = MathHelper::floor_double(_collider.minX) >> 4;
		int maxCX = MathHelper::floor_double(_collider.maxX + 1.0) >> 4;
		int minCZ = MathHelper::floor_double(_collider.minZ) >> 4;
		int maxCZ = MathHelper::floor_double(_collider.maxZ + 1.0) >> 4;

		for (int cx = minCX; cx <= maxCX; cx++) {
			for (int cz = minCZ; cz <= maxCZ; cz++) {
				if (!isChunkValid({ cx, cz }))
					return false;
			}
		}
		return true;
	}

	Int32_2 blockToChunkPos(Int32_2 _blockPos) {
		return { _blockPos.x >> 4, _blockPos.z >> 4 };
	}

	// Returns true when the world-space Y is within valid chunk bounds.
	static constexpr bool inBounds(int _y) {
		return _y >= 0 && _y < CHUNK_HEIGHT;
	}

private:
	// I believe the vanilla default is
	int VIEW_RADIUS = 9;
	int SIMULATION_RADIUS = 9;

	bool isHell = false; // for the nether

	void seedChunkLighting(Int32_2 _pos);

	std::shared_ptr<Chunk> getChunkShared(Int32_2 _pos) {
		auto it = chunks.find(_pos);
		return (it != chunks.end()) ? it->second : nullptr;
	}

	// Check if a chunk can be populated
	bool canPopulateDirect(Int32_2 _pos) {
		auto* chunk = getChunkRaw(_pos);
		if (!chunk)
			return false;
		if (chunk->isTerrainPopulated)
			return false;
		if (chunk->state.load() < ChunkState::Generated)
			return false;
		auto* a = getChunkRaw({ _pos.x + 1, _pos.z });
		auto* b = getChunkRaw({ _pos.x, _pos.z + 1 });
		auto* c = getChunkRaw({ _pos.x + 1, _pos.z + 1 });
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