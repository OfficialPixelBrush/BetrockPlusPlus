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
	std::shared_ptr<Chunk> m_chunks[3][3];

	std::shared_ptr<Chunk> getChunk(Int2 pos) const {
		if (pos.m_x < -1 || pos.m_x > 1 || pos.m_z < -1 || pos.m_z > 1)
			return nullptr;
		return m_chunks[pos.m_x + 1][pos.m_z + 1];
	}
};

// Wrapper for world access during chunk population.
// Holds a 3x3 region of chunk pointers centered on the chunk being populated.
// Chunks are marked inUse on acquire and released on free.
struct WorldWrapper {
	WorldManager& m_manager;
	ChunkPtrRegion m_chunkRegion;
	Int2 m_centerChunkPos;

	// Grab the 3x3 region. Any chunk that is already inUse is left as nullptr
	// (writes to it will fall through to the deferred path via the m_manager).
	void getChunkRegion() {
		for (int dx = -1; dx <= 1; dx++) {
			for (int dz = -1; dz <= 1; dz++) {
				int ax = m_centerChunkPos.m_x + dx;
				int az = m_centerChunkPos.m_z + dz;
				auto c = m_manager.getChunk({ ax, az });
				if (!c || c->m_inUse.load()) {
					m_chunkRegion.m_chunks[dx + 1][dz + 1] = nullptr;
				} else {
					m_chunkRegion.m_chunks[dx + 1][dz + 1] = c;
				}
			}
		}
		// Mark all successfully acquired chunks as inUse
		for (auto& row : m_chunkRegion.m_chunks)
			for (auto& c : row)
				if (c)
					c->m_inUse.store(true);
	}

	void freeChunkRegion() {
		for (auto& row : m_chunkRegion.m_chunks)
			for (auto& c : row)
				if (c)
					c->m_inUse.store(false);
	}

	// Convert a world-space position to a region-local chunk offset (-1..1, -1..1)
	Int2 getRegionChunkPos(Int3 wPos) const {
		return { (wPos.m_x >> 4) - m_centerChunkPos.m_x, (wPos.m_z >> 4) - m_centerChunkPos.m_z };
	}

	int findTopSolidBlock(int wx, int wz) {
		auto chunk = m_chunkRegion.getChunk(getRegionChunkPos({ wx, 0, wz }));
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return -1;
		int lx = wx & 15, lz = wz & 15;
		for (int y = 127; y > 0; --y) {
			BlockType block = chunk->getBlock({ lx, y, lz });
			if (block == BlockType::BLOCK_AIR)
				continue;
			Material mat = Blocks::blockProperties[block].m_material;
			if (mat.m_isSolid || mat.m_isLiquid)
				return y + 1;
		}
		return -1;
	}

	int getHeightValue(int wx, int wz) {
		auto chunk = m_chunkRegion.getChunk(getRegionChunkPos({ wx, 0, wz }));
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return 0;
		return chunk->getHeightValue({ wx & 15, wz & 15 });
	}

	double getTemperatureAt(int wx, int wz) {
		auto chunk = m_chunkRegion.getChunk(getRegionChunkPos({ wx, 0, wz }));
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->getTemperature({ wx & 15, wz & 15 }));
	}

	double getHumidityAt(int wx, int wz) {
		auto chunk = m_chunkRegion.getChunk(getRegionChunkPos({ wx, 0, wz }));
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return 0.5;
		return double(chunk->getHumidity({ wx & 15, wz & 15 }));
	}

	BlockType getBlockId(Int3 wpos) const {
		if (!inBounds(wpos.m_y))
			return BlockType::BLOCK_AIR;
		auto chunk = m_chunkRegion.getChunk(getRegionChunkPos(wpos));
		// Falls outside our grabbed region -> ask the m_manager directly (read-only, safe)
		if (!chunk)
			return m_manager.getBlockId(wpos);
		if (chunk->m_state.load() < ChunkState::Generated)
			return BlockType::BLOCK_AIR;
		return chunk->getBlock({ wpos.m_x & 15, wpos.m_y, wpos.m_z & 15 });
	}

	void setBlock(Int3 wpos, BlockType type, uint8_t meta = 0) {
		if (!inBounds(wpos.m_y))
			return;
		auto chunk = m_chunkRegion.getChunk(getRegionChunkPos(wpos));
		if (!chunk || chunk->m_state.load() < ChunkState::Generated) {
			// Outside our locked region
			m_manager.setBlock(wpos, type, meta);
			return;
		}

		// Remove any tile entities that exist at this spot
		auto& tes = chunk->m_tileEntities;
		tes.erase(std::remove_if(tes.begin(), tes.end(),
		                         [&](const std::shared_ptr<TileEntity>& te) { return te && te->m_position == wpos; }),
		          tes.end());

		// Unlight before changing the block
		m_manager.m_lightManager.unlightAt(wpos.m_x, wpos.m_y, wpos.m_z, LightType::Block, m_manager);
		m_manager.m_lightManager.unlightAt(wpos.m_x, wpos.m_y, wpos.m_z, LightType::Sky, m_manager);

		// Get the local coordinates of this block within the chunk and set it
		int lx = wpos.m_x & 15;
		int lz = wpos.m_z & 15;
		Int3 local{ lx, wpos.m_y, lz };
		chunk->setBlock(local, type);
		chunk->setMeta(local, meta);

		int y = wpos.m_y;
		int x = wpos.m_x;
		int z = wpos.m_z;
		int oldHeight = chunk->getHeightValue({ lx, lz });

		if (Blocks::blockProperties[type].m_lightOpacity != 0) {
			// Placing opaque block; heightmap may rise
			if (y >= oldHeight) {
				chunk->relightColumn({ lx, lz });

				// The column below the new top was zeroed out by relightColumn.
				// Notify the BFS that all blocks from y down to oldHeight need updating
				for (int sy = oldHeight; sy <= y; ++sy)
					m_manager.m_lightManager.unlightAt(x, sy, z, LightType::Sky, m_manager);
			}
		} else if (y == oldHeight - 1) {
			// Removing top opaque block; heightmap may fall
			chunk->relightColumn({ lx, lz });
		}

		int newHeight = chunk->getHeightValue({ lx, lz });
		if (newHeight < oldHeight) {
			for (int sy = newHeight; sy < oldHeight; ++sy)
				m_manager.m_lightManager.scheduleLightUpdate({ x, sy, z }, LightType::Sky);
		}

		// Always re-evaluate the edited block and its 4 horizontal neighbours
		// across the height transition band.
		m_manager.m_lightManager.scheduleLightUpdate({ x, y, z }, LightType::Sky);
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
			m_manager.m_lightManager.scheduleLightRegion({ nx, minY, nz }, { nx, maxY, nz }, LightType::Sky);
		}
		// Schedule a block light update for the position itself
		m_manager.m_lightManager.scheduleLightUpdate({ x, y, z }, LightType::Block);

		// Callback for the client and server to know about this block update
		if (m_manager.m_onBlockUpdate)
			m_manager.m_onBlockUpdate(PendingBlock{ .m_block{ type, meta },
			                                      .m_block_pos{ wpos.m_x, wpos.m_y, wpos.m_z },
			                                      .m_light{ chunk->getBlockLight({ wpos.m_x & 15, wpos.m_y, wpos.m_z & 15 }),
			                                              chunk->getSkyLight({ wpos.m_x & 15, wpos.m_y, wpos.m_z & 15 }) } },
			                        chunk->m_cpos);
	}

	uint8_t getSkyLight(Int3 wpos) const {
		if (!inBounds(wpos.m_y))
			return 0;
		auto chunk = m_chunkRegion.getChunk(getRegionChunkPos(wpos));
		if (!chunk || chunk->m_state.load() < ChunkState::Generated)
			return 0;
		return chunk->getSkyLight({ wpos.m_x & 15, wpos.m_y, wpos.m_z & 15 });
	}

	int64_t getSeed() const {
		return m_manager.m_seed;
	}

	// Returns true when the world-space Y is within valid chunk bounds.
	static constexpr bool inBounds(int y) {
		return y >= 0 && y < CHUNK_HEIGHT;
	}
};

// Inline block-property helpers
inline bool IsSolid(BlockType t) {
	return Blocks::blockProperties[t].m_material.m_isSolid;
}
inline bool IsLiquid(BlockType t) {
	return Blocks::blockProperties[t].m_material.m_isLiquid;
}
inline bool IsOpaque(BlockType t) {
	return Blocks::blockProperties[t].m_lightOpacity > 0;
}

// Used for generating features in the world
class FeatureGenerator {
public:
	BlockType m_type = BLOCK_AIR;
	int8_t m_meta = 0;

	FeatureGenerator() {}
	explicit FeatureGenerator(BlockType pType) : m_type(pType) {}
	FeatureGenerator(BlockType pType, int8_t pMeta) : m_type(pType), m_meta(pMeta) {}

	// Overworld features
	bool GenerateLake(WorldWrapper& world, Java::Random& rand, Int3 pos);
	bool GenerateDungeon(WorldWrapper& world, Java::Random& rand, Int3 pos);
	bool GenerateClay(WorldWrapper& world, Java::Random& rand, Int3 pos, int32_t blobSize = 0);
	bool GenerateMinable(WorldWrapper& world, Java::Random& rand, Int3 pos, int32_t blobSize = 0);
	bool GenerateFlowers(WorldWrapper& world, Java::Random& rand, Int3 pos);
	bool GenerateTallgrass(WorldWrapper& world, Java::Random& rand, Int3 pos);
	bool GenerateDeadbush(WorldWrapper& world, Java::Random& rand, Int3 pos);
	bool GenerateSugarcane(WorldWrapper& world, Java::Random& rand, Int3 pos);
	bool GeneratePumpkins(WorldWrapper& world, Java::Random& rand, Int3 pos);
	bool GenerateCacti(WorldWrapper& world, Java::Random& rand, Int3 pos);
	bool GenerateLiquid(WorldWrapper& world, Java::Random& rand, Int3 pos);
	// Nether Features
	bool GenerateNetherLiquid(WorldWrapper& world, Java::Random& rand, Int3 pos);
	bool GenerateNetherFire(WorldWrapper& world, Java::Random& rand, Int3 pos);
	bool GenerateNetherGlowstone(WorldWrapper& world, Java::Random& rand, Int3 pos);

	static ItemStack GenerateDungeonChestLoot(Java::Random& rand);
	static std::string PickMobToSpawn(Java::Random& rand);
};