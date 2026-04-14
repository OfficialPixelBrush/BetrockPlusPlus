/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "BlockProperties.h"

namespace Blocks {

    // Global table definitions; declared extern in the header
    BlockProperties blockProperties[256] = {};
    BlockBehavior   blockBehaviors[256] = {};

    void registerAll() {
        // Stone
        blockProperties[BlockType::BLOCK_STONE] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 1.5f,
            .resistance = 10.0f,
        };

        // Grass
        blockProperties[BlockType::BLOCK_GRASS] = {
            .material = Material::Grass(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 255,
            .hardness = 0.6f,
            .ticksOnLoad = true,
        };

        // Dirt
        blockProperties[BlockType::BLOCK_DIRT] = {
            .material = Material::Ground(),
            .stepSound = StepSound::Gravel,
            .lightOpacity = 255,
            .hardness = 0.5f,
        };

        // Air
        blockProperties[BlockType::BLOCK_AIR] = {
            .material = Material::Air(),
            .lightOpacity = 0,
            .hardness = 0.0f,
            .resistance = 0.0f,
            .isSolid = false,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .canBlockGrass = false,
            .enableStats = false,
        };

        // Bedrock
        blockProperties[BlockType::BLOCK_BEDROCK] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .hardness = -1.0f,    // unbreakable
            .resistance = 6000000.0f,
            .enableStats = false,
        };
    }

} // namespace Blocks