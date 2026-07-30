/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/
#pragma once
#include "chunk.h"
#include "constants.h"
#include "java_random.h"
#include "numeric_structs.h"
#include <deque>
#include <unordered_map>
#include <vector>
/**
 * @brief Used to carve caves into the world
 *
 */
class CaveGenerator {
private:
	static constexpr int32_t M_CARVE_EXTENT_LIMIT = 8;
	static constexpr size_t M_ORIGIN_CACHE_LIMIT = 4096;
	struct CaveTunnelStep {
		Vec3 offset;
		double radiusXz;
		double radiusY;
		bool eligible;
	};
	struct CaveTunnel {
		std::vector<CaveTunnelStep> steps;
		std::vector<CaveTunnel> branches;
		int32_t startStep = 0;
		int32_t tunnelLength = 0;
		float tunnelRadius = 0.0f;
		bool isShaft = false;
	};
	Java::Random rand = Java::Random();
	std::unordered_map<Int2, std::vector<CaveTunnel>> originCache;
	std::deque<Int2> originCacheOrder;
	const std::vector<CaveTunnel>& GetTunnelsForOrigin(Int2 _chunkOffset);
	std::vector<CaveTunnel> RecordOrigin(Int2 _chunkOffset);
	CaveTunnel RecordTunnel(Vec3 _offset, float _tunnelRadius, float _carveYaw, float _carvePitch, int32_t _tunnelStep,
	                        int32_t _tunnelLength, double _verticalScale);
	void ApplyTunnel(Chunk& _chunk, const CaveTunnel& _tunnel);

public:
	CaveGenerator(bool _isNetherCave = false) : isNetherCave(_isNetherCave) {}
	void GenerateCavesForChunk(Chunk& _chunk, int64_t _seed);
	bool isNetherCave = false;
};