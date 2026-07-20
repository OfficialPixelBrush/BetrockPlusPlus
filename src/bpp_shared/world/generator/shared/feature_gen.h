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

	std::shared_ptr<Chunk> getChunk(Int2 _pos) const {
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
	void getChunkRegion() {
		for (int dx = -1; dx <= 1; dx++) {
			for (int dz = -1; dz <= 1; dz++) {
				int ax = centerChunkPos.x + dx;
				int az = centerChunkPos.z + dz;
				auto c = manager.getChunk({ ax, az });
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

	void freeChunkRegion() {
		for (auto& row : chunkRegion.chunks)
			for (auto& c : row)
				if (c)
					c->inUse.store(false);
	}

	// Convert a world-space position to a region-local chunk offset (-1..1, -1..1)
	Int2 getRegionChunkPos(Int3 _wPos) const {
		return { (_wPos.x >> 4) - centerChunkPos.x, (_wPos.z >> 4) - centerChunkPos.z };
	}

	int findTopSolidBlock(int _wx, int _wz) {
		auto chunk = chunkRegion.getChunk(getRegionChunkPos({ _wx, 0, _wz }));
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return -1;
		int lx = _wx & 15, lz = _wz & 15;
		for (int y = 127; y > 0; --y) {
			BlockType block = chunk->getBlock({ lx, y, lz });
			if (block == BlockType::BLOCK_AIR)
				continue;
			Material mat = Blocks::blockProperties[block].material;
			if (mat.isSolid || mat.isLiquid)
				return y + 1;
		}
		return -1;
	}

	int getHeightValue(int _wx, int _wz) {
		auto chunk = chunkRegion.getChunk(getRegionChunkPos({ _wx, 0, _wz }));
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->getHeightValue({ _wx & 15, _wz & 15 });
	}

	double getTemperatureAt(int _wx, int _wz) {
		auto chunk = chunkRegion.getChunk(getRegionChunkPos({ _wx, 0, _wz }));
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->getTemperature({ _wx & 15, _wz & 15 }));
	}

	double getHumidityAt(int _wx, int _wz) {
		auto chunk = chunkRegion.getChunk(getRegionChunkPos({ _wx, 0, _wz }));
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->getHumidity({ _wx & 15, _wz & 15 }));
	}

	BlockType getBlockId(Int3 _wpos) const {
		if (!inBounds(_wpos.y))
			return BlockType::BLOCK_AIR;
		auto chunk = chunkRegion.getChunk(getRegionChunkPos(_wpos));
		// Falls outside our grabbed region -> ask the manager directly (read-only, safe)
		if (!chunk)
			return manager.getBlockId(_wpos);
		if (chunk->state.load() < ChunkState::Generated)
			return BlockType::BLOCK_AIR;
		return chunk->getBlock({ _wpos.x & 15, _wpos.y, _wpos.z & 15 });
	}

	void setBlock(Int3 _wpos, BlockType _type, uint8_t _meta = 0) {
		if (!inBounds(_wpos.y))
			return;
		auto chunk = chunkRegion.getChunk(getRegionChunkPos(_wpos));
		if (!chunk || chunk->state.load() < ChunkState::Generated) {
			// Outside our locked region
			manager.setBlock(_wpos, _type, _meta);
			return;
		}

		// Remove any tile entities that exist at this spot
		auto& tes = chunk->tileEntities;
		tes.erase(std::remove_if(tes.begin(), tes.end(),
		                         [&](const std::shared_ptr<TileEntity>& _te) { return _te && _te->position == _wpos; }),
		          tes.end());

		// Unlight before changing the block
		manager.lightManager.unlightAt(_wpos.x, _wpos.y, _wpos.z, LightType::Block, manager);
		manager.lightManager.unlightAt(_wpos.x, _wpos.y, _wpos.z, LightType::Sky, manager);

		// Get the local coordinates of this block within the chunk and set it
		int lx = _wpos.x & 15;
		int lz = _wpos.z & 15;
		Int3 local{ lx, _wpos.y, lz };
		chunk->setBlock(local, _type);
		chunk->setMeta(local, _meta);

		int y = _wpos.y;
		int x = _wpos.x;
		int z = _wpos.z;
		int oldHeight = chunk->getHeightValue({ lx, lz });

		if (Blocks::blockProperties[_type].lightOpacity != 0) {
			// Placing opaque block; heightmap may rise
			if (y >= oldHeight) {
				chunk->relightColumn({ lx, lz });

				// The column below the new top was zeroed out by relightColumn.
				// Notify the BFS that all blocks from y down to oldHeight need updating
				for (int sy = oldHeight; sy <= y; ++sy)
					manager.lightManager.unlightAt(x, sy, z, LightType::Sky, manager);
			}
		} else if (y == oldHeight - 1) {
			// Removing top opaque block; heightmap may fall
			chunk->relightColumn({ lx, lz });
		}

		int newHeight = chunk->getHeightValue({ lx, lz });
		if (newHeight < oldHeight) {
			for (int sy = newHeight; sy < oldHeight; ++sy)
				manager.lightManager.scheduleLightUpdate({ x, sy, z }, LightType::Sky);
		}

		// Always re-evaluate the edited block and its 4 horizontal neighbours
		// across the height transition band.
		manager.lightManager.scheduleLightUpdate({ x, y, z }, LightType::Sky);
		const int ndx[] = { -1, 1, 0, 0 };
		const int ndz[] = { 0, 0, -1, 1 };
		for (int i = 0; i < 4; ++i) {
			int nx = x + ndx[i], nz = z + ndz[i];
			int neighborHeight = getHeightValue(nx, nz);
			int thisHeight = chunk->getHeightValue({ lx, lz });
			if (neighborHeight == thisHeight)
				continue;
			int minY = CrossPlatform::Math::min(thisHeight, neighborHeight);
			int maxY = CrossPlatform::Math::max(thisHeight, neighborHeight);
			manager.lightManager.scheduleLightRegion({ nx, minY, nz }, { nx, maxY, nz }, LightType::Sky);
		}
		// Schedule a block light update for the position itself
		manager.lightManager.scheduleLightUpdate({ x, y, z }, LightType::Block);

		// Callback for the client and server to know about this block update
		if (manager.onBlockUpdate)
			manager.onBlockUpdate(PendingBlock{ .block{ _type, _meta },
			                                      .block_pos{ _wpos.x, _wpos.y, _wpos.z },
			                                      .light{ chunk->getBlockLight({ _wpos.x & 15, _wpos.y, _wpos.z & 15 }),
			                                              chunk->getSkyLight({ _wpos.x & 15, _wpos.y, _wpos.z & 15 }) } },
			                        chunk->cpos);
	}

	uint8_t getSkyLight(Int3 _wpos) const {
		if (!inBounds(_wpos.y))
			return 0;
		auto chunk = chunkRegion.getChunk(getRegionChunkPos(_wpos));
		if (!chunk || chunk->state.load() < ChunkState::Generated)
			return 0;
		return chunk->getSkyLight({ _wpos.x & 15, _wpos.y, _wpos.z & 15 });
	}

	int64_t getSeed() const {
		return manager.seed;
	}

	// Returns true when the world-space Y is within valid chunk bounds.
	static constexpr bool inBounds(int _y) {
		return _y >= 0 && _y < CHUNK_HEIGHT;
	}
};

// Inline block-property helpers
inline bool IsSolid(BlockType _t) {
	return Blocks::blockProperties[_t].material.isSolid;
}
inline bool IsLiquid(BlockType _t) {
	return Blocks::blockProperties[_t].material.isLiquid;
}
inline bool IsOpaque(BlockType _t) {
	return Blocks::blockProperties[_t].lightOpacity > 0;
}

// Used for generating features in the world
class FeatureGenerator {
public:
	BlockType type = BLOCK_AIR;
	int8_t meta = 0;

	FeatureGenerator() {}
	explicit FeatureGenerator(BlockType _pType) : type(_pType) {}
	FeatureGenerator(BlockType _pType, int8_t _pMeta) : type(_pType), meta(_pMeta) {}

	// Overworld features
	bool GenerateLake(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateDungeon(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateClay(WorldWrapper& _world, Java::Random& _rand, Int3 _pos, int32_t _blobSize = 0);
	bool GenerateMinable(WorldWrapper& _world, Java::Random& _rand, Int3 _pos, int32_t _blobSize = 0);
	bool GenerateFlowers(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateTallgrass(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateDeadbush(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateSugarcane(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GeneratePumpkins(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateCacti(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateLiquid(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	// Nether Features
	bool GenerateNetherLiquid(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateNetherFire(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);
	bool GenerateNetherGlowstone(WorldWrapper& _world, Java::Random& _rand, Int3 _pos);

	static ItemStack GenerateDungeonChestLoot(Java::Random& _rand);
	static std::string PickMobToSpawn(Java::Random& _rand);
};