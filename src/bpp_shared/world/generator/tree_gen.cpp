/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#include "tree_gen.h"
#include <cstdint>

/**
 * @brief Attempts to generate an oak or birch tree.
 * 
 * @param world Pointer to the world where it'll generate
 * @param rand Pointer to the Java::Random that'll be utilized
 * @param pos Position of the lowest trunk-block
 * @param birch If the tree should be birch or oak
 * @return If tree successfully generated
 */
bool TreeGenerator::Generate(World *world, Java::Random& rand, Int3 pos, bool birch) {
	// Decide on the tree height (birches are one block taller)
	int32_t treeHeight = rand.nextInt(3) + 4;
	if (birch)
		treeHeight++;
	
	// Check if there's space to generate the tree
	// If any block in the desired area is neither air or
	// leaves, we fail the placement check
	if (pos.y >= 1 && pos.y + treeHeight + 1 <= CHUNK_HEIGHT) {
		Int3 check;
		for (check.y = pos.y; check.y <= pos.y + 1 + treeHeight; ++check.y) {
			int8_t width = 1;
			if (check.y == pos.y)
				width = 0;

			if (check.y >= pos.y + 1 + treeHeight - 2)
				width = 2;

			for (check.x = pos.x - width; check.x <= pos.x + width; ++check.x) {
				for (check.z = pos.z - width; check.z <= pos.z + width; ++check.z) {
					// Only test blocks that're within chunk boundaries
					if (check.y >= 0 && check.y < CHUNK_HEIGHT) {
						BlockType blockTest = world->GetBlockType(check);
						if (blockTest != BLOCK_AIR && blockTest != BLOCK_LEAVES) {
							return false;
						}
					} else {
						return false;
					}
				}
			}
		}

		// Check if the bock under the source block is grass or dirt
		BlockType soilType = world->GetBlockType(Int3{pos.x, pos.y - 1, pos.z});
		if ((soilType == BLOCK_GRASS || soilType == BLOCK_DIRT) && pos.y < CHUNK_HEIGHT - treeHeight - 1) {
			// Replace the underlying block with dir
			world->SetBlockType(BLOCK_DIRT, Int3{pos.x, pos.y - 1, pos.z});

			// Place leaves
			Int3 offset;
			for (offset.y = pos.y - 3 + treeHeight; offset.y <= pos.y + treeHeight; ++offset.y) {
				int32_t widthBase = offset.y - (pos.y + treeHeight);
				int32_t treeWidth = 1 - widthBase / 2;

				for (offset.x = pos.x - treeWidth; offset.x <= pos.x + treeWidth; ++offset.x) {
					int32_t xLeaf = offset.x - pos.x;

					for (offset.z = pos.z - treeWidth; offset.z <= pos.z + treeWidth; ++offset.z) {
						int32_t zLeaf = offset.z - pos.z;
						// Leaves are placed within the tree width
						// and replace any non-opaque block
						if ((
								(
								JavaMath::abs(xLeaf) != treeWidth ||
								JavaMath::abs(zLeaf) != treeWidth ||
								(rand.nextInt(2) != 0 && widthBase != 0)
							)) && !IsOpaque(
								world->GetBlockType(offset))) {
							world->PlaceBlock(offset, BLOCK_LEAVES, birch ? 2 : 0);
						}
					}
				}
			}

			// Replace air and leaves with trunk
			for (int32_t h = 0; h < treeHeight; ++h) {
				BlockType futureLog = world->GetBlockType(Int3{pos.x, pos.y + h, pos.z});
				if (futureLog == BLOCK_AIR || futureLog == BLOCK_LEAVES) {
					world->PlaceBlock(Int3{pos.x, pos.y + h, pos.z}, BLOCK_LOG, birch ? 2 : 0);
				}
			}

			return true;
		}
		return false;
	}
	return false;
}

/**
 * @brief Configures the settings of a BigTree
 * 
 * @param pTreeHeight Sets the maximum tree height (this is internally multiplied by 12)
 * @param pBranchLength Sets the maximum branch length
 * @param pTrunkShape Determines the trunk shape
 */
void BigTreeGenerator::Configure(double pTreeHeight, double pBranchLength, double pTrunkShape) {
	this->maximumTreeHeight = Java::DoubleToInt32(pTreeHeight * 12.0);
	if (pTreeHeight > 0.5) {
		this->trunkThickness = 5;
	}

	this->branchLength = pBranchLength;
	this->trunkShape = pTrunkShape;
}


/**
 * @brief Attempts to generate a big oak tree
 * 
 * @param pWorld Pointer to the world where it'll generate
 * @param pRand Pointer to the Java::Random that'll be utilized
 * @param pPos Position of the lowest trunk-block
 * @param pBirch If the tree should be birch or oak (not used for big trees)
 * @return If tree successfully generated
 */
bool BigTreeGenerator::Generate([[maybe_unused]] World *pWorld, Java::Random& pRand, [[maybe_unused]] Int3 pPos, [[maybe_unused]] bool pBirch) {
	this->world = pWorld;
	int64_t seed = pRand.nextLong();
	this->rand.setSeed(seed);
	this->basePos = pPos;
	// If the height wasn't set, generate a new random height
	if (this->totalHeight == 0) {
		this->totalHeight = 5 + this->rand.nextInt(this->maximumTreeHeight);
	}

	// Check if tree can be placed
	if (!this->ValidPlacement())
		return false;
	this->GenerateBranchPositions();
	this->GenerateLeafClusters();
	this->GenerateTrunk();
	this->GenerateBranches();
	return true;
}

/**
 * @brief Determine where branches will go
 * 
 */
void BigTreeGenerator::GenerateBranchPositions() {
	this->height = Java::DoubleToInt32(double(this->totalHeight) * this->heightFactor);
	if (this->height >= this->totalHeight) {
		this->height = this->totalHeight - 1;
	}

	int32_t branchesPerLayer = Java::DoubleToInt32(1.382 + std::pow(this->trunkShape * double(this->totalHeight) / 13.0, 2.0));
	if (branchesPerLayer < 1) {
		branchesPerLayer = 1;
	}

	std::vector<BranchPos> candidateBranches(branchesPerLayer * this->totalHeight);
	int32_t currentY = this->basePos.y + this->totalHeight - this->trunkThickness;
	int32_t branchCount = 1;
	int32_t targetY = this->basePos.y + this->height;
	int32_t canpoyLayer = currentY - this->basePos.y;
	candidateBranches[0].pos.x = this->basePos.x;
	candidateBranches[0].pos.y = currentY;
	candidateBranches[0].pos.z = this->basePos.z;
	candidateBranches[0].trunkY = targetY;
	--currentY;

	while (true) {
		for (; canpoyLayer >= 0; --canpoyLayer) {
			float canopyRadius = this->GetCanopyRadius(canpoyLayer);
			if (canopyRadius >= 0.0F) {
				// Attempt to generate Branch
				for (int32_t attempts = 0; attempts < branchesPerLayer; ++attempts) {
					double radialDistance =
						this->branchLength *
						double(canopyRadius) *
						(double(this->rand.nextFloat()) + 0.328);
					// Oh hey, look! An approximation of pi!
					double angle = double(this->rand.nextFloat()) * 2.0 * 3.14159;
					int32_t branchX = MathHelper::floor_double(
						radialDistance * double(std::sin(angle)) +
						double(this->basePos.x) + 0.5
					);
					int32_t branchZ = MathHelper::floor_double(
						radialDistance * double(std::cos(angle)) +
						double(this->basePos.z) + 0.5
					);
					Int3 branchBase = Int3{branchX, currentY, branchZ};
					Int3 branchTop = Int3{branchX, currentY + this->trunkThickness, branchZ};
					if (this->CheckIfPathClear(branchBase, branchTop) == -1) {
						Int3 trunkConnection = this->basePos;
						double horizontalDistance = std::sqrt(
								std::pow(double(JavaMath::abs(this->basePos.x - branchBase.x)), 2.0) +
								std::pow(double(JavaMath::abs(this->basePos.z - branchBase.z)), 2.0)
							);
						double verticalDrop = horizontalDistance * this->field_752_i;
						if ((double)branchBase.y - verticalDrop > (double)targetY) {
							trunkConnection.y = targetY;
						} else {
							trunkConnection.y = Java::DoubleToInt32(double(branchBase.y) - verticalDrop);
						}

						if (this->CheckIfPathClear(trunkConnection, branchBase) == -1) {
							candidateBranches[branchCount].pos.x = branchX;
							candidateBranches[branchCount].pos.y = currentY;
							candidateBranches[branchCount].pos.z = branchZ;
							candidateBranches[branchCount].trunkY = trunkConnection.y;
							++branchCount;
						}
					}
				}
			}
			--currentY;
		}

		this->branchStartEnd = std::vector<BranchPos>(branchCount);
		std::copy(candidateBranches.begin(), candidateBranches.begin() + branchCount, this->branchStartEnd.begin());
		// Not present in OG Code, but probably good to do
		this->branchStartEnd.shrink_to_fit();
		return;
	}
}

/**
 * @brief Place a circle of blocks along the specified plane
 * 
 * @param centerPos Center position of the circle
 * @param radius Radius of the circle
 * @param axis Axis along which the circle will grow
 * @param blockType Blocktype of the circle
 */
void BigTreeGenerator::PlaceCircularLayer(Int3 centerPos, float radius, BranchAxis axis, BlockType blockType) {
	int32_t intRadius = Java::DoubleToInt32(double(radius) + 0.618);
	BranchAxis axisU = branchOrientation[axis];
	BranchAxis axisV = branchOrientation[axis + AXIS_OFFSET];
	Int3 currentPos{0, 0, 0};

	for (int32_t du = -intRadius; du <= intRadius; ++du) {
		currentPos[axis] = centerPos[axis];
		currentPos[axisU] = centerPos[axisU] + du;

		for (int32_t dv = -intRadius; dv <= intRadius; ++dv) {
			// Pythagorean Theorem
			double distance = std::sqrt(
				std::pow(JavaMath::abs(du) + 0.5, 2.0) +
				std::pow(JavaMath::abs(dv) + 0.5, 2.0)
			);

			if (distance > double(radius))
				continue;

			currentPos[axisV] = centerPos[axisV] + dv;
			BlockType otherType = this->world->GetBlockType(currentPos);

			// Only place if block can be overwritten
			if (otherType == BLOCK_AIR || otherType == BLOCK_LEAVES) {
				this->world->PlaceBlock(currentPos, blockType);
			}
		}
	}
}

/**
 * @brief Get the radius of the leaf canopy at the desired height
 * 
 * @param y Height to check
 * @return Radius in blocks
 */
float BigTreeGenerator::GetCanopyRadius(int32_t y) {
	if ((double)y < double((float)this->totalHeight) * 0.3) {
		return -1.618F;
	} else {
		float halfHeight = (float)this->totalHeight / 2.0F;
		float distanceFromCenter = (float)this->totalHeight / 2.0F - (float)y;
		float radius;
		if (distanceFromCenter == 0.0F) {
			radius = halfHeight;
		} else if (JavaMath::abs(distanceFromCenter) >= halfHeight) {
			radius = 0.0F;
		} else {
			radius = (float)std::sqrt(
				std::pow((double)JavaMath::abs(halfHeight), 2.0) -
				std::pow((double)JavaMath::abs(distanceFromCenter), 2.0));
		}

		radius *= 0.5F;
		return radius;
	}
}

/**
 * @brief Get the radius of the current trunk layer
 * 
 * @param layerIndex The index of the layer
 * @return float Trunk layer radius
 */
float BigTreeGenerator::GetTrunkLayerRadius(int32_t layerIndex) {
    if (layerIndex < 0 || layerIndex >= this->trunkThickness)
        return -1.0f;
    // Top and bottom of trunk are thinner
    if (layerIndex == 0 || layerIndex == this->trunkThickness - 1)
        return 2.0f;
    // Middle of trunk is thicker
    return 3.0f;
}

/**
 * @brief Iterate over the height 
 * 
 * @param base 
 */
void BigTreeGenerator::PlaceLeavesAroundPoint(Int3 base) {
	int32_t baseHeight = base.y + this->trunkThickness;
	
	for (int32_t y = base.y; y < baseHeight; ++y) {
		float trunkRadius = this->GetTrunkLayerRadius(y - base.y);
		this->PlaceCircularLayer(
			Int3{base.x, y, base.z},
			trunkRadius,
			AXIS_Y, 
			BLOCK_LEAVES
		);
	}
}

/**
 * @brief Draws a line of blockType between two coordinates
 * 
 * @param startPos The start position
 * @param endPos The end position
 * @param blockType The block that should be drawn along this line
 */
void BigTreeGenerator::DrawBlockLine(Int3 startPos, Int3 endPos, BlockType blockType) {
	Int3 delta = INT3_ZERO;
	BranchAxis dominantAxis = AXIS_X;
	delta = endPos - startPos;
	// Determine which axis was the largest magnitude
	for (int8_t axis = 0; axis < 3; axis++) {
		if (JavaMath::abs(delta[axis]) > JavaMath::abs(delta[dominantAxis])) {
			dominantAxis = BranchAxis(axis);
		}
	}
	// If an axis was chosen, we can continue
	if (delta[dominantAxis] == 0)
		return;
	// Determine secondary axes
	// X -> Y/Z
	// Y -> X/Z
	// Z -> X/Y
	BranchAxis secondaryA = branchOrientation[dominantAxis];
	BranchAxis secondaryB = branchOrientation[dominantAxis + AXIS_OFFSET];
	int8_t step = -1;
	if (delta[dominantAxis] > 0)
		step = 1;

	double secondaryRatioA = (double)delta[secondaryA] / (double)delta[dominantAxis];
	double secondaryRatioB = (double)delta[secondaryB] / (double)delta[dominantAxis];
	Int3 blockPos = INT3_ZERO;
	int32_t distanceAlongAxis = 0;

	for (int32_t totalSteps = delta[dominantAxis] + step; distanceAlongAxis != totalSteps; distanceAlongAxis += step) {
		blockPos[dominantAxis] = MathHelper::floor_double(
			double(startPos[dominantAxis] + distanceAlongAxis) + 0.5
		);
		blockPos[secondaryA] = MathHelper::floor_double(
			double(startPos[secondaryA]) + double(distanceAlongAxis) * secondaryRatioA + 0.5
		);
		blockPos[secondaryB] = MathHelper::floor_double(
			double(startPos[secondaryB]) + double(distanceAlongAxis) * secondaryRatioB + 0.5
		);
		this->world->PlaceBlock(blockPos, blockType);
	}
}

/**
 * @brief Iterate through the branches and generate the leaves
 * 
 */
void BigTreeGenerator::GenerateLeafClusters() {
	size_t maxBranchNodes = this->branchStartEnd.size();
	for (size_t i = 0; i < maxBranchNodes; ++i) {
		Int3 pos = this->branchStartEnd[i].pos;
		this->PlaceLeavesAroundPoint(pos);
	}
}

bool BigTreeGenerator::CanGenerateBranchAtHeight(int32_t y) {
	return double(y) >= (double(this->totalHeight) * 0.2);
}

void BigTreeGenerator::GenerateTrunk() {
	Int3 startPos = this->basePos;
	Int3 endPos = this->basePos + Int3{0,this->height,0};
	this->DrawBlockLine(startPos, endPos, BLOCK_LOG);
	if (this->branchDensity == 2) {
		startPos.x++;
		endPos.x++;
		this->DrawBlockLine(startPos, endPos, BLOCK_LOG);
		startPos.z++;
		endPos.z++;
		this->DrawBlockLine(startPos, endPos, BLOCK_LOG);
		startPos.x--;
		endPos.x--;
		this->DrawBlockLine(startPos, endPos, BLOCK_LOG);
	}
}

void BigTreeGenerator::GenerateBranches() {
	Int3 base = this->basePos;
	for (size_t branchIndex = 0; branchIndex < this->branchStartEnd.size(); ++branchIndex) {
		Int3 branchPos = this->branchStartEnd[branchIndex].pos;
		int32_t trunkY = this->branchStartEnd[branchIndex].trunkY;
		base.y = trunkY;
		int32_t yHeight = base.y - this->basePos.y;
		if (this->CanGenerateBranchAtHeight(yHeight)) {
			this->DrawBlockLine(base, branchPos, BLOCK_LOG);
		}
	}
}

/**
 * @brief Check if the path is unobstructed between start and end in a straight line
 * 
 * @param startPos The start position
 * @param endPos The end position
 * @return int32_t 
 */
int32_t BigTreeGenerator::CheckIfPathClear(Int3 startPos, Int3 endPos) {
	Int3 delta = INT3_ZERO;

	BranchAxis dominantAxis = AXIS_X;
	delta = endPos - startPos;
	for (int8_t axis = 0; axis < 3; ++axis) {
		if (JavaMath::abs(delta[axis]) > JavaMath::abs(delta[dominantAxis])) {
			dominantAxis = BranchAxis(axis);
		}
	}

	if (delta[dominantAxis] == 0)
		return -1;
	// Determine secondary axes
	BranchAxis secondaryA = branchOrientation[dominantAxis];
	BranchAxis secondaryB = branchOrientation[dominantAxis + AXIS_OFFSET];
	int8_t step = -1;
	if (delta[dominantAxis] > 0)
		step = 1;

	double secondaryRatioA = double(delta[secondaryA]) / double(delta[dominantAxis]);
	double secondaryRatioB = double(delta[secondaryB]) / double(delta[dominantAxis]);
	Int3 currentPos = INT3_ZERO;
	int32_t distanceAlongAxis = 0;

	int32_t totalSteps = delta[dominantAxis] + step;
	for (; distanceAlongAxis != totalSteps; distanceAlongAxis += step) {
		currentPos[dominantAxis] = startPos[dominantAxis] + distanceAlongAxis;
		currentPos[secondaryA] = MathHelper::floor_double(
			double(startPos[secondaryA]) + double(distanceAlongAxis) * secondaryRatioA
		);
		currentPos[secondaryB] = MathHelper::floor_double(
			double(startPos[secondaryB]) + double(distanceAlongAxis) * secondaryRatioB
		);
		BlockType blockType = this->world->GetBlockType(currentPos);
		if (blockType != BLOCK_AIR && blockType != BLOCK_LEAVES) {
			break;
		}
	}

	if (distanceAlongAxis == totalSteps)
		return -1;
	return JavaMath::abs(distanceAlongAxis);
}

/**
 * @brief Test if the desired tree placement is valid along the vertical axis
 * 
 * @return If the placement is valid
 */
bool BigTreeGenerator::ValidPlacement() {
	Int3 endPos = Int3{this->basePos.x, this->basePos.y + this->totalHeight - 1, this->basePos.z};
	// Check if ground block is valid
	BlockType soilType = this->world->GetBlockType(Int3{this->basePos.x, this->basePos.y - 1, this->basePos.z});
	if (soilType != BLOCK_GRASS && soilType != BLOCK_DIRT)
		return false;
	int32_t clearLength = this->CheckIfPathClear(this->basePos, endPos);
	// Path isn't clear
	if (clearLength == -1)
		return true;
	// Path is too short
	if (clearLength < 6)
		return false;
	// Path is valid
	this->totalHeight = clearLength;
	return true;
}

/**
 * @brief Attempts to generate a taiga tree
 * 
 * @param pWorld Pointer to the world where it'll generate
 * @param pRand Pointer to the Java::Random that'll be utilized
 * @param pPos Position of the lowest trunk-block
 * @param pBirch If the tree should be birch or oak (not used for taiga trees)
 * @return If tree successfully generated
 */
bool TaigaTreeGenerator::Generate(World *world, Java::Random& rand, Int3 pos, [[maybe_unused]] bool birch) {
	int32_t height = rand.nextInt(5) + 7;
	int32_t trunkHeight  = height - rand.nextInt(2) - 3;
	int32_t leavesHeight = height - trunkHeight ;
	int32_t maxLeafRadius = 1 + rand.nextInt(leavesHeight + 1);
	// Check if tree can be placed
	if (pos.y >= 1 && pos.y + height + 1 <= CHUNK_HEIGHT) {
		int32_t currentLeafRadius;
		for (int32_t y = pos.y; y <= pos.y + 1 + height; ++y) {
			if (y - pos.y < trunkHeight ) {
				currentLeafRadius = 0;
			} else {
				currentLeafRadius = maxLeafRadius;
			}

			for (int32_t x = pos.x - currentLeafRadius; x <= pos.x + currentLeafRadius; ++x) {
				for (int32_t z = pos.z - currentLeafRadius; z <= pos.z + currentLeafRadius; ++z) {
					if (y >= 0 && y < CHUNK_HEIGHT) {
						BlockType blockType = world->GetBlockType(Int3{x, y, z});
						if (blockType != BLOCK_AIR && blockType != BLOCK_LEAVES) {
							return false;
						}
					} else {
						return false;
					}
				}
			}
		}

		// Check if we're attempting to place it on a valid soil block
		BlockType soilType = world->GetBlockType(Int3{pos.x, pos.y - 1, pos.z});
		if ((soilType == BLOCK_GRASS || soilType == BLOCK_DIRT) && pos.y < CHUNK_HEIGHT - height - 1) {
			world->SetBlockType(BLOCK_DIRT, Int3{pos.x, pos.y - 1, pos.z});
			currentLeafRadius = 0;
			// Generate the leaves
			for (int32_t y = pos.y + height; y >= pos.y + trunkHeight ; --y) {
				for (int32_t x = pos.x - currentLeafRadius; x <= pos.x + currentLeafRadius; ++x) {
					int32_t xOffset = x - pos.x;

					for (int32_t z = pos.z - currentLeafRadius; z <= pos.z + currentLeafRadius; ++z) {
						int32_t zOffset = z - pos.z;
						if ((JavaMath::abs(xOffset) != currentLeafRadius || JavaMath::abs(zOffset) != currentLeafRadius || currentLeafRadius <= 0) &&
							!IsOpaque(world->GetBlockType(Int3{x, y, z}))) {
							// Spruce leaves
							world->SetBlockTypeAndMeta(BLOCK_LEAVES, 1, Int3{x, y, z});
						}
					}
				}

				if (currentLeafRadius >= 1 && y == pos.y + trunkHeight  + 1) {
					--currentLeafRadius;
				} else if (currentLeafRadius < maxLeafRadius) {
					++currentLeafRadius;
				}
			}
			// Place the trunk
			for (int32_t logY = 0; logY < height - 1; ++logY) {
				BlockType type = world->GetBlockType(Int3{pos.x, pos.y + logY, pos.z});
				if (type == BLOCK_AIR || type == BLOCK_LEAVES) {
					world->SetBlockTypeAndMeta(BLOCK_LOG, 1, Int3{pos.x, pos.y + logY, pos.z});
				}
			}

			return true;
		}
		return false;
	}
	return false;
}


/**
 * @brief Attempts to generate an alt taiga tree
 * 
 * @param pWorld Pointer to the world where it'll generate
 * @param pRand Pointer to the Java::Random that'll be utilized
 * @param pPos Position of the lowest trunk-block
 * @param pBirch If the tree should be birch or oak (not used for alt taiga trees)
 * @return If tree successfully generated
 */
bool AltTaigaTreeGenerator::Generate(World *world, Java::Random& rand, Int3 pos, [[maybe_unused]] bool birch) {
	int32_t height = rand.nextInt(4) + 6;
	int32_t trunkHeight = 1 + rand.nextInt(2);
	int32_t leavesHeight = height - trunkHeight;
	int32_t maxLeafRadius = 2 + rand.nextInt(2);
	// Check if tree can be placed
	if (pos.y >= 1 && pos.y + height + 1 <= CHUNK_HEIGHT) {
		for (int32_t y = pos.y; y <= pos.y + 1 + height; ++y) {
			int32_t currentLeafRadius = maxLeafRadius;
			if (y - pos.y < trunkHeight) {
				currentLeafRadius = 0;
			}

			for (int32_t xOffset = pos.x - currentLeafRadius; xOffset <= pos.x + currentLeafRadius; ++xOffset) {
				for (int32_t zOffset = pos.z - currentLeafRadius; zOffset <= pos.z + currentLeafRadius; ++zOffset) {
					if (y >= 0 && y < CHUNK_HEIGHT) {
						BlockType type = world->GetBlockType(Int3{xOffset, y, zOffset});
						if (type != 0 && type != BLOCK_LEAVES) {
							return false;
						}
					} else {
						return false;
					}
				}
			}
		}
	
		// Check if we're attempting to place it on a valid soil block
		BlockType soilType = world->GetBlockType(Int3{pos.x, pos.y - 1, pos.z});
		if ((soilType == BLOCK_GRASS || soilType == BLOCK_DIRT) && pos.y < CHUNK_HEIGHT - height - 1) {
			world->SetBlockType(BLOCK_DIRT, Int3{pos.x, pos.y - 1, pos.z});
			int32_t currentLeafRadius = rand.nextInt(2);
			int32_t leafRadiusIncrementThreshold = 1;
			int32_t leafRadiusSwitch = 0;

			for (int32_t leafLayer  = 0; leafLayer  <= leavesHeight; ++leafLayer ) {
				int32_t yLevel = pos.y + height - leafLayer ;

				for (int32_t x = pos.x - currentLeafRadius; x <= pos.x + currentLeafRadius; ++x) {
					int32_t xOffset = x - pos.x;

					for (int32_t z = pos.z - currentLeafRadius; z <= pos.z + currentLeafRadius; ++z) {
						int32_t zOffset = z - pos.z;
						if ((JavaMath::abs(xOffset) != currentLeafRadius || JavaMath::abs(zOffset) != currentLeafRadius || currentLeafRadius <= 0) &&
							!IsOpaque(world->GetBlockType(Int3{x, yLevel, z}))) {
							world->SetBlockTypeAndMeta(BLOCK_LEAVES, 1, Int3{x, yLevel, z});
						}
					}
				}

				if (currentLeafRadius >= leafRadiusIncrementThreshold) {
					currentLeafRadius = leafRadiusSwitch;
					leafRadiusSwitch = 1;
					++leafRadiusIncrementThreshold;
					if (leafRadiusIncrementThreshold > maxLeafRadius) {
						leafRadiusIncrementThreshold = maxLeafRadius;
					}
				} else {
					++currentLeafRadius;
				}
			}

			int32_t logOffset = rand.nextInt(3);
			for (int32_t logY = 0; logY < height - logOffset; ++logY) {
				BlockType type = world->GetBlockType(Int3{pos.x, pos.y + logY, pos.z});
				if (type == BLOCK_AIR || type == BLOCK_LEAVES) {
					world->SetBlockTypeAndMeta(BLOCK_LOG, 1, Int3{pos.x, pos.y + logY, pos.z});
				}
			}

			return true;
		}
		return false;
	}
	return false;
}