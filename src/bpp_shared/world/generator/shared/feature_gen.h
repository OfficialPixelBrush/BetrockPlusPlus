/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once

#include "blocks/block_properties.h"
#include "constants.h"
#include "java_random.h"
#include "world/world.h"

// 3x3 region of chunk pointers, centered on the chunk being populated
struct ChunkPtrRegion {
	std::shared_ptr<Chunk> chunks[3][3];

	std::shared_ptr<Chunk> GetChunk(Int2 _pos) const {
		if (_pos.x < -1 || _pos.x > 1 || _pos.z < -1 || _pos.z > 1)
			return nullptr;
		return chunks[_pos.x + 1][_pos.z + 1];
	}
};

// Wrapper for world access during chunk population.
// Holds a 3x3 region of chunk pointers centered on the chunk being populated.
// Chunks are marked inUse on acquire and released on free.
struct WorldWrapper {
	WorldManager& manager;
	ChunkPtrRegion chunkRegion;
	Int2 centerChunkPos;

	// Grab the 3x3 region. Any chunk that is already inUse is left as nullptr
	// (writes to it will fall through to the deferred path via the manager).
	void GetChunkRegion() {
		for (int dx = -1; dx <= 1; dx++) {
			for (int dz = -1; dz <= 1; dz++) {
				int ax = centerChunkPos.x + dx;
				int az = centerChunkPos.z + dz;
				auto c = manager.GetChunk({ ax, az });
				if (!c || c->inUse.load()) {
					chunkRegion.chunks[dx + 1][dz + 1] = nullptr;
				} else {
					chunkRegion.chunks[dx + 1][dz + 1] = c;
				}
			}
		}
		// Mark all successfully acquired chunks as inUse
		for (auto& row : chunkRegion.chunks)
			for (auto& c : row)
				if (c)
					c->inUse.store(true);
	}

	void FreeChunkRegion() {
		for (auto& row : chunkRegion.chunks)
			for (auto& c : row)
				if (c)
					c->inUse.store(false);
	}

	// Convert a world-space position to a region-local chunk offset (-1..1, -1..1)
	Int2 GetRegionChunkPos(const Int3 _wPos) const {
		return { (_wPos.x >> 4) - centerChunkPos.x, (_wPos.z >> 4) - centerChunkPos.z };
	}

	int FindTopSolidBlock(const int _wx, const int _wz) {
		auto chunk = chunkRegion.GetChunk(GetRegionChunkPos({ _wx, 0, _wz }));
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return -1;
		int lx = _wx & 15, lz = _wz & 15;
		for (int y = 127; y > 0; --y) {
			BlockType block = chunk->GetBlock({ lx, y, lz });
			if (block == BlockType::BLOCK_AIR)
				continue;
			Material mat = Blocks::blockProperties[block].material;
			if (mat.isSolid || mat.isLiquid)
				return y + 1;
		}
		return -1;
	}

	int GetHeightValue(const int _wx, const int _wz) {
		auto chunk = chunkRegion.GetChunk(GetRegionChunkPos({ _wx, 0, _wz }));
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->GetHeightValue({ _wx & 15, _wz & 15 });
	}

	double GetTemperatureAt(const int _wx, const int _wz) {
		auto chunk = chunkRegion.GetChunk(GetRegionChunkPos({ _wx, 0, _wz }));
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->GetTemperature({ _wx & 15, _wz & 15 }));
	}

	double GetHumidityAt(const int _wx, const int _wz) {
		auto chunk = chunkRegion.GetChunk(GetRegionChunkPos({ _wx, 0, _wz }));
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->GetHumidity({ _wx & 15, _wz & 15 }));
	}

	BlockType GetBlockId(const Int3 _wpos) const {
		if (!InBounds(_wpos.y))
			return BlockType::BLOCK_AIR;
		auto chunk = chunkRegion.GetChunk(GetRegionChunkPos(_wpos));
		// Falls outside our grabbed region -> ask the manager directly (read-only, safe)
		if (!chunk)
			return manager.GetBlockId(_wpos);
		if (chunk->state.load() < ChunkState::Generated)
			return BlockType::BLOCK_AIR;
		return chunk->GetBlock({ _wpos.x & 15, _wpos.y, _wpos.z & 15 });
	}

	void SetBlock(const Int3 _wpos, const BlockType _type, const uint8_t _meta = 0) {
		if (!InBounds(_wpos.y))
			return;
		auto chunk = chunkRegion.GetChunk(GetRegionChunkPos(_wpos));
		if (!chunk || chunk->state.load() < ChunkState::Generated) {
			// Outside our locked region
			manager.SetBlock(_wpos, _type, _meta);
			return;
		}

		// Remove any tile entities that exist at this spot
		auto& tes = chunk->tileEntities;
		if (!tes.empty()) {
			tes.erase(std::remove_if(tes.begin(), tes.end(),
			                         [&](const std::shared_ptr<TileEntity>& _te) {
				                         return _te && _te->position == _wpos;
			                         }),
			          tes.end());
		}

		// Get the local coordinates of this block within the chunk and set it
		int lx = _wpos.x & 15;
		int lz = _wpos.z & 15;
		Int3 local{ lx, _wpos.y, lz };
		auto oldBlock = chunk->GetBlock(local);
		//auto oldMeta = chunk->GetMeta(local);

		// Making the assumption here that certain metadatas of
		// blocks don't have differing light properties
		bool changesLighting = (Blocks::blockProperties[_type].lightOpacity !=
		                        Blocks::blockProperties[oldBlock].lightOpacity) ||
		                       (Blocks::blockProperties[_type].lightEmission !=
		                        Blocks::blockProperties[oldBlock].lightEmission);

		// Unlight before changing the block
		if (changesLighting) {
			manager.lightManager.UnlightAt(_wpos.x, _wpos.y, _wpos.z, LightType::Block, manager);
			manager.lightManager.UnlightAt(_wpos.x, _wpos.y, _wpos.z, LightType::Sky, manager);
		}

		// Then finally set the new block
		chunk->SetBlock(local, _type);
		chunk->SetMeta(local, _meta);

		int y = _wpos.y;
		int x = _wpos.x;
		int z = _wpos.z;
		int oldHeight = chunk->GetHeightValue({ lx, lz });

		// Placing opaque block; heightmap may rise
		if (changesLighting) {
			chunk->RelightColumn({ lx, lz });
			int newHeight = chunk->GetHeightValue({ lx, lz });
			if (newHeight > oldHeight) {
				// Notify the BFS that all blocks from y down to oldHeight need updating
				for (int sy = oldHeight; sy <= newHeight; ++sy) {
					manager.lightManager.UnlightAt(x, sy, z, LightType::Sky, manager);
				}
			} else if (newHeight < oldHeight) {
				// Height fell
				for (int sy = newHeight; sy < oldHeight; ++sy) {
					manager.lightManager.ScheduleLightUpdate({ x, sy, z }, LightType::Sky);
				}
			}

			// Always re-evaluate the edited block and its 4 horizontal neighbours
			manager.lightManager.ScheduleLightUpdate({ x, y, z }, LightType::Sky);
			manager.lightManager.ScheduleLightUpdate({ x, y, z }, LightType::Block);
			int extendedBottom = CrossPlatform::Math::Min(newHeight, oldHeight);
			while (extendedBottom > 0 &&
			       Blocks::blockProperties[chunk->GetBlock({ lx, extendedBottom - 1, lz })].lightOpacity == 0)
				--extendedBottom;

			if (newHeight != oldHeight) {
				manager.lightManager.ScheduleLightRegion(
				    { x - 1, extendedBottom, z - 1 }, { x + 1, CrossPlatform::Math::Max(newHeight, oldHeight), z + 1 },
				    LightType::Sky);
			}
		}

		// Callback for the client and server to know about this block update
		if (manager.onBlockUpdate)
			manager.onBlockUpdate(PendingBlock{ .block{ _type, _meta },
			                                    .blockPos{ _wpos.x, _wpos.y, _wpos.z },
			                                    .light{ chunk->GetBlockLight({ _wpos.x & 15, _wpos.y, _wpos.z & 15 }),
			                                            chunk->GetSkyLight({ _wpos.x & 15, _wpos.y, _wpos.z & 15 }) } },
			                      chunk->cpos);
	}

	uint8_t GetSkyLight(const Int3 _wpos) const {
		if (!InBounds(_wpos.y))
			return 0;
		auto chunk = chunkRegion.GetChunk(GetRegionChunkPos(_wpos));
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->GetSkyLight({ _wpos.x & 15, _wpos.y, _wpos.z & 15 });
	}

	int64_t GetSeed() const {
		return manager.seed;
	}

	// Returns true when the world-space Y is within valid chunk bounds.
	static constexpr bool InBounds(int _y) {
		return _y >= 0 && _y < CHUNK_HEIGHT;
	}
};

// Inline block-property helpers
inline constexpr bool IsSolid(BlockType _t) {
	return Blocks::blockProperties[_t].material.isSolid;
}
inline constexpr bool IsLiquid(BlockType _t) {
	return Blocks::blockProperties[_t].material.isLiquid;
}
inline constexpr bool IsOpaque(BlockType _t) {
	return Blocks::blockProperties[_t].lightOpacity > 0;
}

// Used for generating features in the world
namespace FeatureGenerator {
	// Overworld features
	bool GenerateLake(BlockType _type, WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateDungeon(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateClay(WorldWrapper& _world, Java::Random& _rand, Int3 _pos, int32_t _blobSize = 0);
	bool GenerateMinable(BlockType _type, WorldWrapper& _world, Java::Random& _rand, Int3 _pos, int32_t _blobSize = 0);
	bool GenerateFlowers(BlockType _type, WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateTallgrass(uint8_t _meta, WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateDeadbush(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateSugarcane(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GeneratePumpkins(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateCacti(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateLiquid(BlockType _type, WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	// Nether Features
	bool GenerateNetherLiquid(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateNetherFire(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateNetherGlowstone(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);

	ItemStack GenerateDungeonChestLoot(Java::Random& _rand);
	std::string PickMobToSpawn(Java::Random& _rand);
};