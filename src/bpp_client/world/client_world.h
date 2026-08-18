/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "world.h"
#include "world_access.h"

// The client world is very simple, used for client side prediction and such
class ClientWorld : public WorldAccess {
public:
	// We only store what the client gives us
	std::unordered_map<Int32_2, std::shared_ptr<Chunk>> chunks;

	ClientWorld(WorldManager& _manager, Int2 _centerChunkPos) : manager(_manager), centerChunkPos(_centerChunkPos) {}

	int FindTopSolidBlock(const int _wx, const int _wz) override;
	int GetHeightValue(const int _wx, const int _wz) override;
	double GetTemperatureAt(const int _wx, const int _wz) override;

	double GetHumidityAt(const int _wx, const int _wz) override;
	BlockType GetBlockId(const Int3 _wpos) override;
	void SetBlock(const Int3 _wpos, const BlockType _type, const uint8_t _meta = 0, const bool _keepTileEntity = false,
	              const bool _updateNeighbors = true) override;
	uint8_t GetSkyLight(const Int3 _wpos) override;
	int GetBlockLightRaw(const Int3 _wpos) override;

	std::shared_ptr<Chunk> GetChunkShared(Int2 _pos) {
		auto it = chunks.find(_pos);
		return (it != chunks.end()) ? it->second : nullptr;
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
};