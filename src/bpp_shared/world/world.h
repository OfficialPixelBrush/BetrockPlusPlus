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
	Int3 blockPos{ 0, 0, 0 };
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
	TickTime elapsedTicks = 0;
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

	void Tick(const std::vector<ClientPosition>& _players);
	void Update(const std::vector<ClientPosition>& _players);
	void Shutdown();
	void SeedChunkLighting(Int32_2 _pos);
	std::vector<AABB> GetCollidingBoundingBoxes(const AABB& _area);
	void FlushBleedWrites();
	void PropagateChunkLightBorders(Int32_2 _cpos);
	BlockType GetFirstUncoveredBlock(int _wx, int _wz);
	int FindTopSolidBlock(int _wx, int _wz);
	void SetMeta(Int3 _wpos, uint8_t _metadata = 0);
	void SetBlock(Int3 _wpos, BlockType _blockType, uint8_t _metadata = 0);
	void DrainGenQueue();
	bool IsLiquidInAabb(AABB _collider);
	void InitSpawn();
	bool HandleFluidAcceleration(AABB _collider, Material _material, Entity& _entity);
	bool IsMaterialInAabb(AABB _collider, Material _material);
	void UpdateLoadRadius(const std::vector<ClientPosition>& _players);
	void PumpPipeline(const std::vector<ClientPosition>& _players);
	void PopulateReady();
	void DrainLoadQueue();

	const int GetViewRadius() {
		return viewRadius;
	}
	const int GetSimulationDistance() {
		return simulationRadius;
	}
	void SetViewRadius(int _viewRadius) {
		int newViewRadius = std::max(3, _viewRadius);
		this->viewRadius = newViewRadius;
		this->simulationRadius = std::min(9, newViewRadius);
	}

	void InitWorldSeed(std::string _pSeed) {
		this->seed = HashCode(_pSeed);
	}

	void InitWorldSeed(int64_t _pSeed) {
		this->seed = _pSeed;
	}

	void NotifyNeighborsOfUpdate(Int3 _globalPos) {
		// Update our six neighbors
		const int ndx[] = { -1, 1, 0, 0 };
		const int ndz[] = { 0, 0, -1, 1 };

		// Notify horizontal neighbors
		for (int i = 0; i < 4; i++) {
			auto dx = ndx[i];
			auto dz = ndz[i];
			Int3 newPos = { _globalPos.x + dx, _globalPos.y, _globalPos.z + dz };
			auto block = this->GetBlockId(newPos);
			auto updateFunction = Blocks::blockBehaviors[block].onNeighborBlockChange;
			if (updateFunction)
				updateFunction(*this, newPos);
		}

		// Vertical neighbors
		for (int i = 0; i < 2; i++) {
			auto dy = ndx[i]; // we are using ndx because the first two items are -1, 1
			Int3 newPos = { _globalPos.x, _globalPos.y + dy, _globalPos.z };
			auto block = this->GetBlockId(newPos);
			auto updateFunction = Blocks::blockBehaviors[block].onNeighborBlockChange;
			if (updateFunction)
				updateFunction(*this, newPos);
		}
	}

	// For creating a fresh tile entity for generation etc
	void CreateTileEntity(std::shared_ptr<TileEntity> _tileEntity) {
		Int32_2 cpos{ _tileEntity->position.x >> 4, _tileEntity->position.z >> 4 };
		Chunk* chunk = GetChunkRaw(cpos);
		if (!chunk)
			return;
		_tileEntity->chunk = chunk;
		tileEntityManager.InitializeTileEntity(_tileEntity);   // weak_ptr added if canTick
		chunk->tileEntities.push_back(std::move(_tileEntity)); // chunk takes ownership
	}

	// For registering a tile entity that already exists in the world (e.g. loaded from disk)
	void RegisterChunkTileEntities(Chunk* _chunk) {
		for (auto& te : _chunk->tileEntities) {
			if (te) {
				tileEntityManager.InitializeTileEntity(te);
				te->chunk = _chunk;
			}
		}
	}

	// Returns the tile entity at world position `pos`, or nullptr if none.
	TileEntity* GetTileEntity(Int3 _pos) {
		Chunk* chunk = GetChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
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
	T* GetTileEntityAs(Int3 _pos) {
		return dynamic_cast<T*>(GetTileEntity(_pos));
	}

	template <typename T>
	std::shared_ptr<T> GetTileEntityShared(Int3 _pos) {
		Chunk* chunk = GetChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
		if (!chunk)
			return nullptr;
		for (auto& te : chunk->tileEntities) {
			if (te && te->position.x == _pos.x && te->position.y == _pos.y && te->position.z == _pos.z)
				return std::dynamic_pointer_cast<T>(te);
		}
		return nullptr;
	}

	// Remove the tile entity at world position `pos`.
	void RemoveTileEntity(Int3 _pos) {
		Chunk* chunk = GetChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
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
	void PostGenResult(std::shared_ptr<Chunk> _chunk) {
		std::lock_guard lk(genDoneMutex);
		genDoneQueue.push_back(std::move(_chunk));
	}

	std::shared_ptr<Chunk> GetChunk(Int32_2 _pos) {
		return GetChunkShared(_pos);
	}

	bool CanPopulate(Int32_2 _pos) {
		return CanPopulateDirect(_pos);
	}

	BlockType GetBlockId(Int3 _wpos) {
		if (!InBounds(_wpos.y))
			return BlockType::BLOCK_AIR;
		auto* chunk = GetChunkRaw({ _wpos.x >> 4, _wpos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return BlockType::BLOCK_AIR;
		return chunk->GetBlock({ _wpos.x & 15, _wpos.y, _wpos.z & 15 });
	}

	uint8_t GetMetadata(Int3 _wpos) {
		if (!InBounds(_wpos.y))
			return 0;
		auto* chunk = GetChunkRaw({ _wpos.x >> 4, _wpos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->GetMeta({ _wpos.x & 15, _wpos.y, _wpos.z & 15 });
	}

	void SetBlock(Int3 _wpos, const Block& _block) {
		SetBlock(_wpos, _block.type, _block.data);
	}

	bool IsAirBlock(Int3 _wpos) {
		return GetBlockId(_wpos) == BlockType::BLOCK_AIR;
	}

	Material GetMaterial(Int3 _wpos) {
		return Blocks::blockProperties[GetBlockId(_wpos)].material;
	}

	bool IsBlockNormalCube(Int3 _wpos) {
		BlockType block = GetBlockId(_wpos);
		if (block == BlockType::BLOCK_AIR)
			return false;
		const auto& props = Blocks::blockProperties[block];
		return props.material.isSolid && props.isNormalCube;
	}

	Int3 GetSpawnPoint(bool _adjust) {
		if (!_adjust)
			return spawnPoint;
		int sx = this->spawnPoint.x;
		int sz = this->spawnPoint.z;
		sx += rand.NextInt(20) - 10;
		sz += rand.NextInt(20) - 10;
		int sy = FindTopSolidBlock(sx, sz);
		return { sx, sy, sz };
	}

	int GetHeightValue(int _wx, int _wz) {
		auto* chunk = GetChunkRaw({ _wx >> 4, _wz >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->GetHeightValue({ _wx & 15, _wz & 15 });
	}

	// Returns the baked temperature/humidity for a world column.
	double GetTemperatureAt(int _wx, int _wz) {
		auto* chunk = GetChunkRaw({ _wx >> 4, _wz >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->GetTemperature({ _wx & 15, _wz & 15 }));
	}

	double GetHumidityAt(int _wx, int _wz) {
		auto* chunk = GetChunkRaw({ _wx >> 4, _wz >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->GetHumidity({ _wx & 15, _wz & 15 }));
	}

	int GetSkyLight(Int3 _pos) {
		if (!InBounds(_pos.y))
			return 0;
		auto* chunk = GetChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->GetSkyLight({ _pos.x & 15, _pos.y, _pos.z & 15 });
	}

	int GetBlockLight(Int3 _pos) {
		if (!InBounds(_pos.y))
			return 0;
		auto* chunk = GetChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->GetBlockLight({ _pos.x & 15, _pos.y, _pos.z & 15 });
	}

	Chunk* GetChunkRaw(Int32_2 _pos) {
		auto it = chunks.find(_pos);
		return (it != chunks.end()) ? it->second.get() : nullptr;
	}

	bool IsChunkValid(Int32_2 _pos) {
		auto* chunk = GetChunkRaw({ _pos.x, _pos.z });
		if (!chunk)
			return false;
		if (chunk->state.load() >= ChunkState::Generated)
			return true;
		return false;
	}

	bool AABBinValidChunks(AABB _collider) {
		if (_collider.minY < 0.0 || _collider.maxY >= 128.0)
			return false;
		int minCX = MathHelper::FloorDouble(_collider.minX) >> 4;
		int maxCX = MathHelper::FloorDouble(_collider.maxX + 1.0) >> 4;
		int minCZ = MathHelper::FloorDouble(_collider.minZ) >> 4;
		int maxCZ = MathHelper::FloorDouble(_collider.maxZ + 1.0) >> 4;

		for (int cx = minCX; cx <= maxCX; cx++) {
			for (int cz = minCZ; cz <= maxCZ; cz++) {
				if (!IsChunkValid({ cx, cz }))
					return false;
			}
		}
		return true;
	}

	Int32_2 BlockToChunkPos(Int32_2 _blockPos) {
		return { _blockPos.x >> 4, _blockPos.z >> 4 };
	}

	// Returns true when the world-space Y is within valid chunk bounds.
	static constexpr bool InBounds(int _y) {
		return _y >= 0 && _y < CHUNK_HEIGHT;
	}

private:
	// I believe the vanilla default is
	int viewRadius = 9;
	int simulationRadius = 9;

	bool isHell = false; // for the nether

	std::shared_ptr<Chunk> GetChunkShared(Int32_2 _pos) {
		auto it = chunks.find(_pos);
		return (it != chunks.end()) ? it->second : nullptr;
	}

	// Check if a chunk can be populated
	bool CanPopulateDirect(Int32_2 _pos) {
		auto* chunk = GetChunkRaw(_pos);
		if (!chunk)
			return false;
		if (chunk->isTerrainPopulated)
			return false;
		if (chunk->state.load() < ChunkState::Generated)
			return false;
		auto* a = GetChunkRaw({ _pos.x + 1, _pos.z });
		auto* b = GetChunkRaw({ _pos.x, _pos.z + 1 });
		auto* c = GetChunkRaw({ _pos.x + 1, _pos.z + 1 });
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