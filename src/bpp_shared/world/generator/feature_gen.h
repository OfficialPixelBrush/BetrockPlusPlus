/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once

#include "java_random.h"
#include "world/world.h"
#include "base_structs.h"
#include "blocks/block_properties.h"
#include "constants.h"
#include "helpers/java/java_math.h"

// ---------------------------------------------------------------------------
// Inline block-property helpers
// ---------------------------------------------------------------------------
inline bool IsSolid(BlockType t)  { return Blocks::blockProperties[t].material.isSolid;  }
inline bool IsLiquid(BlockType t) { return Blocks::blockProperties[t].material.isLiquid; }
inline bool IsOpaque(BlockType t) { return Blocks::blockProperties[t].lightOpacity > 0;  }

// ---------------------------------------------------------------------------
// FeatureGenerator — Beta 1.7.3 world-decoration features
// ---------------------------------------------------------------------------
class FeatureGenerator {
  public:
    BlockType type = BLOCK_AIR;
    int8_t meta = 0;

    FeatureGenerator() {}
    explicit FeatureGenerator(BlockType pType) : type(pType) {}
    FeatureGenerator(BlockType pType, int8_t pMeta) : type(pType), meta(pMeta) {}

    bool GenerateLake    (WorldManager& world, Java::Random& rand, Int3 pos);
    bool GenerateDungeon (WorldManager& world, Java::Random& rand, Int3 pos);
    bool GenerateClay    (WorldManager& world, Java::Random& rand, Int3 pos, int32_t blobSize = 0);
    bool GenerateMinable (WorldManager& world, Java::Random& rand, Int3 pos, int32_t blobSize = 0);
    bool GenerateFlowers (WorldManager& world, Java::Random& rand, Int3 pos);
    bool GenerateTallgrass(WorldManager& world, Java::Random& rand, Int3 pos);
    bool GenerateDeadbush(WorldManager& world, Java::Random& rand, Int3 pos);
    bool GenerateSugarcane(WorldManager& world, Java::Random& rand, Int3 pos);
    bool GeneratePumpkins(WorldManager& world, Java::Random& rand, Int3 pos);
    bool GenerateCacti   (WorldManager& world, Java::Random& rand, Int3 pos);
    bool GenerateLiquid  (WorldManager& world, Java::Random& rand, Int3 pos);

    static int         GenerateDungeonChestLoot(Java::Random& rand);
    static std::string PickMobToSpawn(Java::Random& rand);
};
