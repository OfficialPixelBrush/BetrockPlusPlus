/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once

#include "../shared/feature_gen.h" // brings in GenView, IsSolid, IsOpaque
#include "java_random.h"

/**
 * @brief Used for generating Oak or Birch Trees
 * 
 */
class TreeGenerator {
public:
	TreeGenerator() {};
	virtual ~TreeGenerator() = default;
	virtual bool Generate(WorldWrapper& _world, Java::Random& _rand, Int3 _pos, bool _birch = false);
	virtual void Configure([[maybe_unused]] double _treeHeight, [[maybe_unused]] double _branchLength,
	                       [[maybe_unused]] double _trunkShape) {};
};

/**
 * @brief Used for generating Big Oak Trees
 * 
 */
class BigTreeGenerator : public TreeGenerator {
private:
	struct BranchPos {
		Int3 pos;
		int32_t trunkY;
	};
	enum BranchAxis : int8_t {
		AXIS_X = 0,
		AXIS_Y = 1,
		AXIS_Z = 2,
		//TRUNK_Y = 3
	};
	static constexpr int8_t AXIS_OFFSET = 3;
	static constexpr BranchAxis BRANCH_ORIENTATION[6] = {
		AXIS_Z, // X to Z
		AXIS_X, // Y to X
		AXIS_X, // Z to X
		AXIS_Y, // X to Y
		AXIS_Z, // Y to Z
		AXIS_Y  // Z to Y
	};
	Java::Random rand = Java::Random();
	WorldWrapper* wm = nullptr;
	Int3 basePos = INT3_ZERO;
	int32_t totalHeight = 0;
	int32_t height;
	double heightFactor = 0.618;
	double field753H = 1.0;
	double trunkSlopeFactor = 0.381;
	double branchLength = 1.0;
	double trunkShape = 1.0;
	int32_t branchDensity = 1;
	int32_t maximumTreeHeight = 12;
	int32_t trunkThickness = 4;
	std::vector<BranchPos> branchStartEnd;

	bool ValidPlacement();
	void GenerateBranchPositions();
	void GenerateLeafClusters();
	void GenerateTrunk();
	void GenerateBranches();
	float GetCanopyRadius(int32_t _y);
	float GetTrunkLayerRadius(int32_t _layerIndex);
	void PlaceLeavesAroundPoint(Int3 _base);
	void DrawBlockLine(Int3 _startPos, Int3 _endPos, BlockType _blockType);
	int32_t CheckIfPathClear(Int3 _startPos, Int3 _endPos);
	void PlaceCircularLayer(Int3 _centerPos, float _radius, BranchAxis _axis, BlockType _blockType);
	bool CanGenerateBranchAtHeight(int32_t _y);

public:
	BigTreeGenerator() {}
	~BigTreeGenerator() = default;
	bool Generate(WorldWrapper& _world, Java::Random& _rand, Int3 _pos, bool _birch = false);
	void Configure(double _treeHeight, double _branchLength, double _trunkShape);
};

/**
 * @brief Used for generating Taiga Trees
 * 
 */
class TaigaTreeGenerator : public TreeGenerator {
public:
	TaigaTreeGenerator() {};
	~TaigaTreeGenerator() = default;
	bool Generate(WorldWrapper& _world, Java::Random& _rand, Int3 _pos, bool _birch = false);
};

/**
 * @brief Used for generating Alternative Taiga Trees
 * 
 */
class AltTaigaTreeGenerator : public TreeGenerator {
public:
	AltTaigaTreeGenerator() {};
	~AltTaigaTreeGenerator() = default;
	bool Generate(WorldWrapper& _world, Java::Random& _rand, Int3 _pos, bool _birch = false);
};