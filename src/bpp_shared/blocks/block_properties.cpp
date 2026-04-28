/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "block_properties.h"

namespace Blocks {

    // Global table definitions; declared extern in the header
    BlockProperties blockProperties[256] = {};
    BlockBehavior   blockBehaviors[256] = {};

    // ================================================================
    //  Behavior helper functions
    //  Must live at namespace scope — cannot be defined inside a function
    // ================================================================

    // --- defaults ---------------------------------------------------

    static AABB defaultAABB(uint8_t) {
        return { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
    }
    static CollisionShape defaultCollider(uint8_t) {
        CollisionShape s;
        s.add({ 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 });
        return s;
    }

    // --- slab -------------------------------------------------------

    static AABB slabAABB(uint8_t) {
        return { 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 };
    }
    static CollisionShape slabCollider(uint8_t) {
        CollisionShape s;
        s.add({ 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 });
        return s;
    }

    // --- stairs -----------------------------------------------------

    static CollisionShape stairCollider(uint8_t meta) {
        CollisionShape s;
        switch (meta & 3) {
        case 0: s.add({ 0.0, 0.0, 0.0, 0.5, 0.5, 1.0 }); s.add({ 0.5, 0.0, 0.0, 1.0, 1.0, 1.0 }); break;
        case 1: s.add({ 0.0, 0.0, 0.0, 0.5, 1.0, 1.0 }); s.add({ 0.5, 0.0, 0.0, 1.0, 0.5, 1.0 }); break;
        case 2: s.add({ 0.0, 0.0, 0.0, 1.0, 0.5, 0.5 }); s.add({ 0.0, 0.0, 0.5, 1.0, 1.0, 1.0 }); break;
        case 3: s.add({ 0.0, 0.0, 0.0, 1.0, 1.0, 0.5 }); s.add({ 0.0, 0.0, 0.5, 1.0, 0.5, 1.0 }); break;
        }
        return s;
    }
    // Stairs use a full-cube ray/selection box — Beta 1.7.3 behaviour

    // --- cactus -----------------------------------------------------

    static AABB cactusAABB(uint8_t) {
        constexpr double I = 0.0625;
        return { I, 0.0, I, 1.0 - I, 1.0, 1.0 - I };
    }
    static CollisionShape cactusCollider(uint8_t) {
        constexpr double I = 0.0625;
        CollisionShape s;
        s.add({ I, 0.0, I, 1.0 - I, 1.0 - I, 1.0 - I });
        return s;
    }

    // --- snow layer -------------------------------------------------

    static AABB snowLayerAABB(uint8_t meta) {
        float h = (2.0f * (1 + (meta & 7))) / 16.0f;
        return { 0.0, 0.0, 0.0, 1.0, h, 1.0 };
    }
    static CollisionShape snowLayerCollider(uint8_t meta) {
        CollisionShape s;
        if ((meta & 7) >= 3)
            s.add({ 0.0, 0.0, 0.0, 1.0, 0.5, 1.0 });
        return s;
    }

    // --- ladder -----------------------------------------------------

    static AABB ladderAABB(uint8_t meta) {
        constexpr double T = 0.125;
        switch (meta) {
        case 2: return { 0.0,     0.0, 1.0 - T, 1.0, 1.0, 1.0 };
        case 3: return { 0.0,     0.0, 0.0,     1.0, 1.0, T };
        case 4: return { 1.0 - T, 0.0, 0.0,     1.0, 1.0, 1.0 };
        case 5: return { 0.0,     0.0, 0.0,     T,   1.0, 1.0 };
        default:return { 0.0,     0.0, 0.0,     1.0, 1.0, 1.0 };
        }
    }
    static CollisionShape ladderCollider(uint8_t meta) {
        constexpr double T = 0.125;
        CollisionShape s;
        switch (meta) {
        case 2: s.add({ 0.0,     0.0, 1.0 - T, 1.0, 1.0, 1.0 }); break;
        case 3: s.add({ 0.0,     0.0, 0.0,     1.0, 1.0, T }); break;
        case 4: s.add({ 1.0 - T, 0.0, 0.0,     1.0, 1.0, 1.0 }); break;
        case 5: s.add({ 0.0,     0.0, 0.0,     T,   1.0, 1.0 }); break;
        }
        return s;
    }

    // --- door -------------------------------------------------------
    // bits 0-1 = facing when closed, bit 2 = open, bit 3 = top half

    static int doorState(uint8_t meta) {
        return ((meta & 4) == 0) ? ((meta - 1) & 3) : (meta & 3);
    }
    static AABB doorAABB(uint8_t meta) {
        constexpr double T = 0.1875;
        switch (doorState(meta)) {
        case 0: return { 0.0,     0.0, 0.0,     1.0, 1.0, T };
        case 1: return { 1.0 - T, 0.0, 0.0,     1.0, 1.0, 1.0 };
        case 2: return { 0.0,     0.0, 1.0 - T, 1.0, 1.0, 1.0 };
        case 3: return { 0.0,     0.0, 0.0,     T,   1.0, 1.0 };
        default:return { 0.0,     0.0, 0.0,     1.0, 1.0, 1.0 };
        }
    }
    static CollisionShape doorCollider(uint8_t meta) {
        constexpr double T = 0.1875;
        CollisionShape s;
        switch (doorState(meta)) {
        case 0: s.add({ 0.0,     0.0, 0.0,     1.0, 1.0, T }); break;
        case 1: s.add({ 1.0 - T, 0.0, 0.0,     1.0, 1.0, 1.0 }); break;
        case 2: s.add({ 0.0,     0.0, 1.0 - T, 1.0, 1.0, 1.0 }); break;
        case 3: s.add({ 0.0,     0.0, 0.0,     T,   1.0, 1.0 }); break;
        }
        return s;
    }

    // --- trapdoor ---------------------------------------------------

    static AABB trapdoorAABB(uint8_t meta) {
        constexpr double T = 0.1875;
        if (!(meta & 4)) return { 0.0, 0.0, 0.0, 1.0, T, 1.0 };
        switch (meta & 3) {
        case 0: return { 0.0,     0.0, 1.0 - T, 1.0, 1.0, 1.0 };
        case 1: return { 0.0,     0.0, 0.0,     1.0, 1.0, T };
        case 2: return { 1.0 - T, 0.0, 0.0,     1.0, 1.0, 1.0 };
        case 3: return { 0.0,     0.0, 0.0,     T,   1.0, 1.0 };
        default:return { 0.0,     0.0, 0.0,     1.0, 1.0, 1.0 };
        }
    }
    static CollisionShape trapdoorCollider(uint8_t meta) {
        constexpr double T = 0.1875;
        CollisionShape s;
        if (!(meta & 4)) { s.add({ 0.0, 0.0, 0.0, 1.0, T, 1.0 }); return s; }
        switch (meta & 3) {
        case 0: s.add({ 0.0,     0.0, 1.0 - T, 1.0, 1.0, 1.0 }); break;
        case 1: s.add({ 0.0,     0.0, 0.0,     1.0, 1.0, T }); break;
        case 2: s.add({ 1.0 - T, 0.0, 0.0,     1.0, 1.0, 1.0 }); break;
        case 3: s.add({ 0.0,     0.0, 0.0,     T,   1.0, 1.0 }); break;
        }
        return s;
    }

    // --- bed --------------------------------------------------------

    static AABB bedAABB(uint8_t) {
        return { 0.0, 0.0, 0.0, 1.0, 0.5625, 1.0 };
    }
    static CollisionShape bedCollider(uint8_t) {
        CollisionShape s;
        s.add({ 0.0, 0.0, 0.0, 1.0, 0.5625, 1.0 });
        return s;
    }

    // --- fence ------------------------------------------------------

    static CollisionShape fenceCollider(uint8_t) {
        CollisionShape s;
        s.add({ 0.0, 0.0, 0.0, 1.0, 1.5, 1.0 });
        return s;
    }

    // --- cake -------------------------------------------------------

    static AABB cakeAABB(uint8_t meta) {
        double x0 = (1 + meta * 2) / 16.0;
        return { x0, 0.0, 0.0625, 1.0 - 0.0625, 0.5 - 0.0625, 1.0 - 0.0625 };
    }
    static CollisionShape cakeCollider(uint8_t meta) {
        double x0 = (1 + meta * 2) / 16.0;
        CollisionShape s;
        s.add({ x0, 0.0, 0.0625, 1.0 - 0.0625, 0.5 - 0.0625, 1.0 - 0.0625 });
        return s;
    }

    // --- repeater ---------------------------------------------------

    static AABB repeaterAABB(uint8_t) {
        return { 0.0, 0.0, 0.0, 1.0, 0.125, 1.0 };
    }
    static CollisionShape emptyCollider(uint8_t) {
        return {};
    }

    // --- button -----------------------------------------------------

    static AABB buttonAABB(uint8_t meta) {
        const int    face = meta & 7;
        const bool   pressed = (meta & 8) != 0;
        constexpr double lo = 0.375, hi = 0.625, hw = 0.1875;
        const double depth = pressed ? 0.0625 : 0.125;
        switch (face) {
        case 1: return { 0.0,         lo, 0.5 - hw,  depth,      hi, 0.5 + hw };
        case 2: return { 1.0 - depth, lo, 0.5 - hw,  1.0,        hi, 0.5 + hw };
        case 3: return { 0.5 - hw,    lo, 0.0,        0.5 + hw,  hi, depth };
        case 4: return { 0.5 - hw,    lo, 1.0 - depth,0.5 + hw,  hi, 1.0 };
        default:return {};
        }
    }

    // --- lever ------------------------------------------------------

    static AABB leverAABB(uint8_t meta) {
        constexpr double f = 0.1875;
        switch (meta & 7) {
        case 1: return { 0.0,         0.2, 0.5 - f,      f * 2.0,      0.8, 0.5 + f };
        case 2: return { 1.0 - f * 2.0, 0.2, 0.5 - f,      1.0,          0.8, 0.5 + f };
        case 3: return { 0.5 - f,     0.2, 0.0,           0.5 + f,      0.8, f * 2.0 };
        case 4: return { 0.5 - f,     0.2, 1.0 - f * 2.0,  0.5 + f,      0.8, 1.0 };
        default: {
            constexpr double g = 0.25;
            return { 0.5 - g, 0.0, 0.5 - g, 0.5 + g, 0.6, 0.5 + g };
        }
        }
    }

    // --- pressure plate ---------------------------------------------

    static AABB pressurePlateAABB(uint8_t meta) {
        constexpr double f = 0.0625;
        return { f, 0.0, f, 1.0 - f, (meta == 1) ? 0.03125 : 0.0625, 1.0 - f };
    }

    // --- torch (normal + redstone, same box) ------------------------

    static AABB torchAABB(uint8_t meta) {
        constexpr float f = 0.15f;
        switch (meta & 7) {
        case 1: return { 0.0f,         0.2f, 0.5f - f,      f * 2.0f,     0.8f, 0.5f + f };
        case 2: return { 1.0f - f * 2.0f,0.2f, 0.5f - f,      1.0f,         0.8f, 0.5f + f };
        case 3: return { 0.5f - f,     0.2f, 0.0f,           0.5f + f,     0.8f, f * 2.0f };
        case 4: return { 0.5f - f,     0.2f, 1.0f - f * 2.0f, 0.5f + f,     0.8f, 1.0f };
        default: {
            constexpr float g = 0.1f;
            return { 0.5f - g, 0.0f, 0.5f - g, 0.5f + g, 0.6f, 0.5f + g };
        }
        }
    }

    // --- rail -------------------------------------------------------

    static AABB railAABB(uint8_t) {
        return { 0.0, 0.0, 0.0, 1.0, 0.125, 1.0 };
    }

    // --- redstone dust ----------------------------------------------

    static AABB redstoneDustAABB(uint8_t) {
        return { 0.0, 0.0, 0.0, 1.0, 0.0625, 1.0 };
    }

    // --- farmland ---------------------------------------------------
    // Collider is full cube; ray/selection use visual height 0.9375

    static AABB farmlandAABB(uint8_t) {
        return { 0.0, 0.0, 0.0, 1.0, 0.9375, 1.0 };
    }

    // --- crop -------------------------------------------------------

    static AABB cropAABB(uint8_t) {
        return { 0.0, 0.0, 0.0, 1.0, 0.25, 1.0 };  // 4/16
    }

    // --- sapling / deadbush (f=0.4) ---------------------------------

    static AABB saplingAABB(uint8_t) {
        constexpr float f = 0.4f;
        return { 0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 2.0f, 0.5f + f };
    }

    // --- tall grass -------------------------------------------------

    static AABB tallGrassAABB(uint8_t) {
        constexpr float f = 0.4f;
        return { 0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.8f, 0.5f + f };
    }

    // --- mushroom (f=0.2) -------------------------------------------

    static AABB mushroomAABB(uint8_t) {
        constexpr float f = 0.2f;
        return { 0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 2.0f, 0.5f + f };
    }

    // --- plant / flower (rose, dandelion) (f=0.2, h=f*3) -----------

    static AABB plantAABB(uint8_t) {
        constexpr float f = 0.2f;
        return { 0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 3.0f, 0.5f + f };
    }

    // --- sugarcane --------------------------------------------------

    static AABB sugarcaneAABB(uint8_t) {
        constexpr float f = 0.375f;
        return { 0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 1.0f, 0.5f + f };
    }

    // --- liquid (zero-size — not selectable) ------------------------

    static AABB liquidAABB(uint8_t) {
        return { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    }

    // --- piston head ------------------------------------------------

    static AABB pistonHeadAABB(uint8_t meta) {
        switch (meta & 7) {
        case 0: return { 0.0,  0.0,  0.0,  1.0,  0.25, 1.0 };
        case 1: return { 0.0,  0.75, 0.0,  1.0,  1.0,  1.0 };
        case 2: return { 0.0,  0.0,  0.0,  1.0,  1.0,  0.25 };
        case 3: return { 0.0,  0.0,  0.75, 1.0,  1.0,  1.0 };
        case 4: return { 0.0,  0.0,  0.0,  0.25, 1.0,  1.0 };
        case 5: return { 0.75, 0.0,  0.0,  1.0,  1.0,  1.0 };
        default:return { 0.0,  0.0,  0.0,  1.0,  1.0,  1.0 };
        }
    }
    static CollisionShape pistonHeadCollider(uint8_t meta) {
        CollisionShape s;
        switch (meta & 7) {
        case 0: s.add({ 0.0,   0.0,   0.0,   1.0,   0.25,  1.0 });
            s.add({ 0.375, 0.25,  0.375, 0.625, 1.0,   0.625 }); break;
        case 1: s.add({ 0.0,   0.75,  0.0,   1.0,   1.0,   1.0 });
            s.add({ 0.375, 0.0,   0.375, 0.625, 0.75,  0.625 }); break;
        case 2: s.add({ 0.0,  0.0,   0.0,  1.0,  1.0,   0.25 });
            s.add({ 0.25, 0.375, 0.25, 0.75, 0.625, 1.0 }); break;
        case 3: s.add({ 0.0,  0.0,   0.75, 1.0,  1.0,   1.0 });
            s.add({ 0.25, 0.375, 0.0,  0.75, 0.625, 0.75 }); break;
        case 4: s.add({ 0.0,   0.0,  0.0,  0.25,  1.0,  1.0 });
            s.add({ 0.375, 0.25, 0.25, 0.625, 0.75, 1.0 }); break;
        case 5: s.add({ 0.75, 0.0,   0.0,  1.0,   1.0,   1.0 });
            s.add({ 0.0,  0.375, 0.25, 0.75,  0.625, 0.75 }); break;
        }
        return s;
    }

    // ================================================================
    //  registerAll
    // ================================================================

    void registerAll() {

        // Default all behavior slots to full-cube before per-block overrides
        for (int i = 0; i < 256; i++) {
            blockBehaviors[i].getSelectionBox = defaultAABB;
            blockBehaviors[i].getRayBounds = defaultAABB;
            blockBehaviors[i].getCollider = defaultCollider;
        }

        // ---- block properties ----------------------------------------

        // Air
        blockProperties[BlockType::BLOCK_AIR] = {
            .material = Material::Air(),
            .lightOpacity = 0,
            .hardness = 0.0f,
            .resistance = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .canBlockGrass = false,
            .enableStats = false,
        };

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

        // Cobblestone
        blockProperties[BlockType::BLOCK_COBBLESTONE] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 2.0f,
            .resistance = 10.0f,
        };

        // Planks (Oak Wood)
        blockProperties[BlockType::BLOCK_PLANKS] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 255,
            .hardness = 2.0f,
            .resistance = 5.0f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Sapling
        blockProperties[BlockType::BLOCK_SAPLING] = {
            .material = Material::Plants(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .ticksOnLoad = true,
            .notifyNeighborsOnMetaChange = false,
        };

        // Bedrock
        blockProperties[BlockType::BLOCK_BEDROCK] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .hardness = -1.0f,    // unbreakable
            .resistance = 6000000.0f,
            .enableStats = false,
        };

        // Water (flowing)
        blockProperties[BlockType::BLOCK_WATER_FLOWING] = {
            .material = Material::Water(),
            .lightOpacity = 3,
            .hardness = 100.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Water (still/stationary)
        blockProperties[BlockType::BLOCK_WATER_STILL] = {
            .material = Material::Water(),
            .lightOpacity = 3,
            .hardness = 100.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Lava (flowing)
        blockProperties[BlockType::BLOCK_LAVA_FLOWING] = {
            .material = Material::Lava(),
            .lightEmission = 15,    // setLightValue(1.0f) -> 15*1.0 = 15
            .lightOpacity = 255,
            .hardness = 0.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Lava (still/stationary)
        blockProperties[BlockType::BLOCK_LAVA_STILL] = {
            .material = Material::Lava(),
            .lightEmission = 15,
            .lightOpacity = 255,
            .hardness = 100.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Sand
        blockProperties[BlockType::BLOCK_SAND] = {
            .material = Material::Sand(),
            .stepSound = StepSound::Sand,
            .lightOpacity = 255,
            .hardness = 0.5f,
        };

        // Gravel
        blockProperties[BlockType::BLOCK_GRAVEL] = {
            .material = Material::Sand(),
            .stepSound = StepSound::Gravel,
            .lightOpacity = 255,
            .hardness = 0.6f,
        };

        // Gold Ore
        blockProperties[BlockType::BLOCK_ORE_GOLD] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 3.0f,
            .resistance = 5.0f,
        };

        // Iron Ore
        blockProperties[BlockType::BLOCK_ORE_IRON] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 3.0f,
            .resistance = 5.0f,
        };

        // Coal Ore
        blockProperties[BlockType::BLOCK_ORE_COAL] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 3.0f,
            .resistance = 5.0f,
        };

        // Wood Log
        blockProperties[BlockType::BLOCK_LOG] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 255,
            .hardness = 2.0f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Leaves
        blockProperties[BlockType::BLOCK_LEAVES] = {
            .material = Material::Leaves(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 1,
            .hardness = 0.2f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .ticksOnLoad = true,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Sponge
        blockProperties[BlockType::BLOCK_SPONGE] = {
            .material = Material::Sponge(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 255,
            .hardness = 0.6f,
        };

        // Glass
        blockProperties[BlockType::BLOCK_GLASS] = {
            .material = Material::Glass(),
            .stepSound = StepSound::Glass,
            .lightOpacity = 0,
            .hardness = 0.3f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Lapis Lazuli Ore
        blockProperties[BlockType::BLOCK_ORE_LAPIS_LAZULI] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 3.0f,
            .resistance = 5.0f,
        };

        // Lapis Lazuli Block
        blockProperties[BlockType::BLOCK_LAPIS_LAZULI] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 3.0f,
            .resistance = 5.0f,
        };

        // Dispenser
        blockProperties[BlockType::BLOCK_DISPENSER] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 3.5f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Sandstone
        blockProperties[BlockType::BLOCK_SANDSTONE] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 0.8f,
        };

        // Note Block
        blockProperties[BlockType::BLOCK_NOTEBLOCK] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 255,
            .hardness = 0.8f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Bed
        blockProperties[BlockType::BLOCK_BED] = {
            .material = Material::Cloth(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 0.2f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Powered Rail (Golden Rail)
        blockProperties[BlockType::BLOCK_RAIL_POWERED] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 0,
            .hardness = 0.7f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Detector Rail
        blockProperties[BlockType::BLOCK_RAIL_DETECTOR] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 0,
            .hardness = 0.7f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Sticky Piston Base
        blockProperties[BlockType::BLOCK_PISTON_STICKY] = {
            .material = Material::Piston(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 0.5f,
            .isOpaqueCube = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Cobweb
        blockProperties[BlockType::BLOCK_COBWEB] = {
            .material = Material::Web(),
            .stepSound = StepSound::Cloth,
            .lightOpacity = 1,
            .hardness = 4.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Tall Grass
        blockProperties[BlockType::BLOCK_TALLGRASS] = {
            .material = Material::Plants(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Dead Bush
        blockProperties[BlockType::BLOCK_DEADBUSH] = {
            .material = Material::Plants(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Piston Base
        blockProperties[BlockType::BLOCK_PISTON] = {
            .material = Material::Piston(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 0.5f,
            .isOpaqueCube = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Piston Extension (head)
        blockProperties[BlockType::BLOCK_PISTON_HEAD] = {
            .material = Material::Piston(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 0,
            .hardness = 0.5f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Wool (Cloth)
        blockProperties[BlockType::BLOCK_WOOL] = {
            .material = Material::Cloth(),
            .stepSound = StepSound::Cloth,
            .lightOpacity = 255,
            .hardness = 0.8f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Piston Moving (tile entity placeholder)
        blockProperties[BlockType::BLOCK_PISTON_MOVING] = {
            .material = Material::Piston(),
            .lightOpacity = 0,
            .hardness = -1.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .enableStats = false,
        };

        // Dandelion (Yellow Flower)
        blockProperties[BlockType::BLOCK_DANDELION] = {
            .material = Material::Plants(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Rose (Red Flower)
        blockProperties[BlockType::BLOCK_ROSE] = {
            .material = Material::Plants(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Brown Mushroom
        blockProperties[BlockType::BLOCK_MUSHROOM_BROWN] = {
            .material = Material::Plants(),
            .stepSound = StepSound::Grass,
            .lightEmission = 1,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Red Mushroom
        blockProperties[BlockType::BLOCK_MUSHROOM_RED] = {
            .material = Material::Plants(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Gold Block
        blockProperties[BlockType::BLOCK_GOLD] = {
            .material = Material::Iron(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 3.0f,
            .resistance = 10.0f,
        };

        // Iron Block
        blockProperties[BlockType::BLOCK_IRON] = {
            .material = Material::Iron(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 5.0f,
            .resistance = 10.0f,
        };

        // Double Stone Slab
        blockProperties[BlockType::BLOCK_DOUBLE_SLAB] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 2.0f,
            .resistance = 10.0f,
        };

        // Stone Slab (single)
        blockProperties[BlockType::BLOCK_SLAB] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 2.0f,
            .resistance = 10.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Bricks
        blockProperties[BlockType::BLOCK_BRICKS] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 2.0f,
            .resistance = 10.0f,
        };

        // TNT
        blockProperties[BlockType::BLOCK_TNT] = {
            .material = Material::TNT(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 255,
            .hardness = 0.0f,
        };

        // Bookshelf
        blockProperties[BlockType::BLOCK_BOOKSHELF] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 255,
            .hardness = 1.5f,
        };

        // Mossy Cobblestone
        blockProperties[BlockType::BLOCK_COBBLESTONE_MOSSY] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 2.0f,
            .resistance = 10.0f,
        };

        // Obsidian
        blockProperties[BlockType::BLOCK_OBSIDIAN] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 10.0f,
            .resistance = 2000.0f,
        };

        // Torch
        blockProperties[BlockType::BLOCK_TORCH] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Wood,
            .lightEmission = 14,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Fire
        blockProperties[BlockType::BLOCK_FIRE] = {
            .material = Material::Fire(),
            .lightEmission = 15,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .canBlockGrass = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Monster Spawner
        blockProperties[BlockType::BLOCK_MOB_SPAWNER] = {
            .material = Material::Iron(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 5.0f,
            .enableStats = false,
        };

        // Oak Wood Stairs
        blockProperties[BlockType::BLOCK_STAIRS_WOOD] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 255,
            .hardness = 2.0f,
            .resistance = 5.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Chest
        blockProperties[BlockType::BLOCK_CHEST] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 2.5f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Redstone Wire
        blockProperties[BlockType::BLOCK_REDSTONE] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Diamond Ore
        blockProperties[BlockType::BLOCK_ORE_DIAMOND] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 3.0f,
            .resistance = 5.0f,
        };

        // Diamond Block
        blockProperties[BlockType::BLOCK_DIAMOND] = {
            .material = Material::Iron(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 5.0f,
            .resistance = 10.0f,
        };

        // Crafting Table (Workbench)
        blockProperties[BlockType::BLOCK_CRAFTING_TABLE] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 255,
            .hardness = 2.5f,
        };

        // Crops / Wheat
        blockProperties[BlockType::BLOCK_CROP_WHEAT] = {
            .material = Material::Plants(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .ticksOnLoad = true,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Farmland (Tilled Field)
        blockProperties[BlockType::BLOCK_FARMLAND] = {
            .material = Material::Ground(),
            .stepSound = StepSound::Gravel,
            .lightOpacity = 255,
            .hardness = 0.6f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Furnace (idle)
        blockProperties[BlockType::BLOCK_FURNACE] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 3.5f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Furnace (active/lit)
        blockProperties[BlockType::BLOCK_FURNACE_LIT] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightEmission = 13,
            .lightOpacity = 255,
            .hardness = 3.5f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Sign (standing)
        blockProperties[BlockType::BLOCK_SIGN] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 1.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Wooden Door
        blockProperties[BlockType::BLOCK_DOOR_WOOD] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 3.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Ladder
        blockProperties[BlockType::BLOCK_LADDER] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 0.4f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Rail (normal)
        blockProperties[BlockType::BLOCK_RAIL] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 0,
            .hardness = 0.7f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Cobblestone Stairs
        blockProperties[BlockType::BLOCK_STAIRS_COBBLESTONE] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 2.0f,
            .resistance = 10.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Wall Sign
        blockProperties[BlockType::BLOCK_SIGN_WALL] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 1.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Lever
        blockProperties[BlockType::BLOCK_LEVER] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 0.5f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Stone Pressure Plate
        blockProperties[BlockType::BLOCK_PRESSURE_PLATE_STONE] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 0,
            .hardness = 0.5f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Iron Door
        blockProperties[BlockType::BLOCK_DOOR_IRON] = {
            .material = Material::Iron(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 0,
            .hardness = 5.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Wooden Pressure Plate
        blockProperties[BlockType::BLOCK_PRESSURE_PLATE_WOOD] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 0.5f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Redstone Ore
        blockProperties[BlockType::BLOCK_ORE_REDSTONE_OFF] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 3.0f,
            .resistance = 5.0f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Redstone Ore (glowing/lit)
        blockProperties[BlockType::BLOCK_ORE_REDSTONE_ON] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightEmission = 9,
            .lightOpacity = 255,
            .hardness = 3.0f,
            .resistance = 5.0f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Redstone Torch (off)
        blockProperties[BlockType::BLOCK_REDSTONE_TORCH_OFF] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Redstone Torch (on)
        blockProperties[BlockType::BLOCK_REDSTONE_TORCH_ON] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Wood,
            .lightEmission = 7,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Stone Button
        blockProperties[BlockType::BLOCK_BUTTON_STONE] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 0,
            .hardness = 0.5f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Snow (layer)
        blockProperties[BlockType::BLOCK_SNOW_LAYER] = {
            .material = Material::Snow(),
            .stepSound = StepSound::Cloth,
            .lightOpacity = 0,
            .hardness = 0.1f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .canBlockGrass = false,
        };

        // Ice
        blockProperties[BlockType::BLOCK_ICE] = {
            .material = Material::Ice(),
            .stepSound = StepSound::Glass,
            .lightOpacity = 3,
            .hardness = 0.5f,
            .slipperiness = 0.98f,
        };

        // Snow Block
        blockProperties[BlockType::BLOCK_SNOW] = {
            .material = Material::BuiltSnow(),
            .stepSound = StepSound::Cloth,
            .lightOpacity = 255,
            .hardness = 0.2f,
        };

        // Cactus
        blockProperties[BlockType::BLOCK_CACTUS] = {
            .material = Material::Cactus(),
            .stepSound = StepSound::Cloth,
            .lightOpacity = 0,
            .hardness = 0.4f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .ticksOnLoad = true,
        };

        // Clay Block
        blockProperties[BlockType::BLOCK_CLAY] = {
            .material = Material::Clay(),
            .stepSound = StepSound::Gravel,
            .lightOpacity = 255,
            .hardness = 0.6f,
        };

        // Sugar Cane (Reed)
        blockProperties[BlockType::BLOCK_SUGARCANE] = {
            .material = Material::Plants(),
            .stepSound = StepSound::Grass,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .ticksOnLoad = true,
            .enableStats = false,
        };

        // Jukebox
        blockProperties[BlockType::BLOCK_JUKEBOX] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 2.0f,
            .resistance = 10.0f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Fence
        blockProperties[BlockType::BLOCK_FENCE] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 2.0f,
            .resistance = 5.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Pumpkin
        blockProperties[BlockType::BLOCK_PUMPKIN] = {
            .material = Material::Pumpkin(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 255,
            .hardness = 1.0f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Netherrack
        blockProperties[BlockType::BLOCK_NETHERRACK] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Stone,
            .lightOpacity = 255,
            .hardness = 0.4f,
        };

        // Soul Sand
        blockProperties[BlockType::BLOCK_SOULSAND] = {
            .material = Material::Sand(),
            .stepSound = StepSound::Sand,
            .lightOpacity = 255,
            .hardness = 0.5f,
        };

        // Glowstone
        blockProperties[BlockType::BLOCK_GLOWSTONE] = {
            .material = Material::Rock(),
            .stepSound = StepSound::Glass,
            .lightEmission = 15,
            .lightOpacity = 255,
            .hardness = 0.3f,
        };

        // Nether Portal
        blockProperties[BlockType::BLOCK_NETHER_PORTAL] = {
            .material = Material::Portal(),
            .stepSound = StepSound::Glass,
            .lightEmission = 11,
            .lightOpacity = 0,
            .hardness = -1.0f,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Jack-o-Lantern (Lit Pumpkin)
        blockProperties[BlockType::BLOCK_PUMPKIN_LIT] = {
            .material = Material::Pumpkin(),
            .stepSound = StepSound::Wood,
            .lightEmission = 15,
            .lightOpacity = 255,
            .hardness = 1.0f,
            .notifyNeighborsOnMetaChange = false,
        };

        // Cake
        blockProperties[BlockType::BLOCK_CAKE] = {
            .material = Material::Cake(),
            .stepSound = StepSound::Cloth,
            .lightOpacity = 0,
            .hardness = 0.5f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Redstone Repeater (off)
        blockProperties[BlockType::BLOCK_REDSTONE_REPEATER_OFF] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Redstone Repeater (on)
        blockProperties[BlockType::BLOCK_REDSTONE_REPEATER_ON] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Wood,
            .lightEmission = 9,
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // Trapdoor
        blockProperties[BlockType::BLOCK_TRAPDOOR] = {
            .material = Material::Wood(),
            .stepSound = StepSound::Wood,
            .lightOpacity = 0,
            .hardness = 3.0f,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
            .enableStats = false,
        };

        // ---- block behaviors (non-default shapes) --------------------

        // Liquids — zero-size AABB (not selectable/collidable)
        blockBehaviors[BlockType::BLOCK_WATER_FLOWING] = {
            .getSelectionBox = liquidAABB,
            .getRayBounds = liquidAABB,
            .getCollider = emptyCollider,
        };
        blockBehaviors[BlockType::BLOCK_WATER_STILL] = {
            .getSelectionBox = liquidAABB,
            .getRayBounds = liquidAABB,
            .getCollider = emptyCollider,
        };
        blockBehaviors[BlockType::BLOCK_LAVA_FLOWING] = {
            .getSelectionBox = liquidAABB,
            .getRayBounds = liquidAABB,
            .getCollider = emptyCollider,
        };
        blockBehaviors[BlockType::BLOCK_LAVA_STILL] = {
            .getSelectionBox = liquidAABB,
            .getRayBounds = liquidAABB,
            .getCollider = emptyCollider,
        };

        // Rails
        blockBehaviors[BlockType::BLOCK_RAIL] = {
            .getRayBounds = railAABB,
            .getCollider = emptyCollider,
        };
        blockBehaviors[BlockType::BLOCK_RAIL_POWERED] = {
            .getRayBounds = railAABB,
            .getCollider = emptyCollider,
        };
        blockBehaviors[BlockType::BLOCK_RAIL_DETECTOR] = {
            .getRayBounds = railAABB,
            .getCollider = emptyCollider,
        };

        // Redstone dust
        blockBehaviors[BlockType::BLOCK_REDSTONE] = {
            .getSelectionBox = redstoneDustAABB,
            .getRayBounds = redstoneDustAABB,
            .getCollider = emptyCollider,
        };

        // Farmland — full-cube collider, short ray/selection box
        blockBehaviors[BlockType::BLOCK_FARMLAND] = {
            .getSelectionBox = farmlandAABB,
            .getRayBounds = farmlandAABB,
            // getCollider stays as defaultCollider (full cube)
        };

        // Crops
        blockBehaviors[BlockType::BLOCK_CROP_WHEAT] = {
            .getSelectionBox = cropAABB,
            .getRayBounds = cropAABB,
            .getCollider = emptyCollider,
        };

        // Sapling
        blockBehaviors[BlockType::BLOCK_SAPLING] = {
            .getSelectionBox = saplingAABB,
            .getRayBounds = saplingAABB,
            .getCollider = emptyCollider,
        };

        // Tall grass
        blockBehaviors[BlockType::BLOCK_TALLGRASS] = {
            .getSelectionBox = tallGrassAABB,
            .getRayBounds = tallGrassAABB,
            .getCollider = emptyCollider,
        };

        // Mushrooms
        blockBehaviors[BlockType::BLOCK_MUSHROOM_BROWN] = {
            .getSelectionBox = mushroomAABB,
            .getRayBounds = mushroomAABB,
            .getCollider = emptyCollider,
        };
        blockBehaviors[BlockType::BLOCK_MUSHROOM_RED] = {
            .getSelectionBox = mushroomAABB,
            .getRayBounds = mushroomAABB,
            .getCollider = emptyCollider,
        };

        // Flowers (rose, dandelion)
        blockBehaviors[BlockType::BLOCK_ROSE] = {
            .getSelectionBox = plantAABB,
            .getRayBounds = plantAABB,
            .getCollider = emptyCollider,
        };
        blockBehaviors[BlockType::BLOCK_DANDELION] = {
            .getSelectionBox = plantAABB,
            .getRayBounds = plantAABB,
            .getCollider = emptyCollider,
        };

        // Dead bush
        blockBehaviors[BlockType::BLOCK_DEADBUSH] = {
            .getSelectionBox = saplingAABB,  // same f=0.4 box as sapling
            .getRayBounds = saplingAABB,
            .getCollider = emptyCollider,
        };

        // Sugar cane
        blockBehaviors[BlockType::BLOCK_SUGARCANE] = {
            .getSelectionBox = sugarcaneAABB,
            .getRayBounds = sugarcaneAABB,
            .getCollider = emptyCollider,
        };

        blockBehaviors[BlockType::BLOCK_SLAB] = {
            .getSelectionBox = slabAABB,
            .getRayBounds = slabAABB,
            .getCollider = slabCollider,
        };

        blockBehaviors[BlockType::BLOCK_STAIRS_WOOD] = {
            .getCollider = stairCollider,
            // ray/selection stay as defaultAABB — full cube is correct
        };
        blockBehaviors[BlockType::BLOCK_STAIRS_COBBLESTONE] = {
            .getCollider = stairCollider,
        };

        blockBehaviors[BlockType::BLOCK_CACTUS] = {
            .getSelectionBox = cactusAABB,
            .getRayBounds = cactusAABB,
            .getCollider = cactusCollider,
        };

        blockBehaviors[BlockType::BLOCK_SNOW_LAYER] = {
            .getRayBounds = snowLayerAABB,
            .getCollider = snowLayerCollider,
            // getSelectionBox stays defaultAABB
        };

        blockBehaviors[BlockType::BLOCK_LADDER] = {
            .getSelectionBox = ladderAABB,
            .getRayBounds = ladderAABB,
            .getCollider = ladderCollider,
        };

        blockBehaviors[BlockType::BLOCK_DOOR_WOOD] = {
            .getSelectionBox = doorAABB,
            .getRayBounds = doorAABB,
            .getCollider = doorCollider,
        };
        blockBehaviors[BlockType::BLOCK_DOOR_IRON] = {
            .getSelectionBox = doorAABB,
            .getRayBounds = doorAABB,
            .getCollider = doorCollider,
        };

        blockBehaviors[BlockType::BLOCK_TRAPDOOR] = {
            .getSelectionBox = trapdoorAABB,
            .getRayBounds = trapdoorAABB,
            .getCollider = trapdoorCollider,
        };

        blockBehaviors[BlockType::BLOCK_BED] = {
            .getSelectionBox = bedAABB,
            .getRayBounds = bedAABB,
            .getCollider = bedCollider,
        };

        blockBehaviors[BlockType::BLOCK_FENCE] = {
            .getCollider = fenceCollider,
            // ray/selection stay as defaultAABB — full cube
        };

        blockBehaviors[BlockType::BLOCK_CAKE] = {
            .getSelectionBox = cakeAABB,
            .getRayBounds = cakeAABB,
            .getCollider = cakeCollider,
        };

        blockBehaviors[BlockType::BLOCK_REDSTONE_REPEATER_OFF] = {
            .getSelectionBox = repeaterAABB,
            .getRayBounds = repeaterAABB,
            .getCollider = emptyCollider,
        };
        blockBehaviors[BlockType::BLOCK_REDSTONE_REPEATER_ON] = {
            .getSelectionBox = repeaterAABB,
            .getRayBounds = repeaterAABB,
            .getCollider = emptyCollider,
        };

        blockBehaviors[BlockType::BLOCK_BUTTON_STONE] = {
            .getSelectionBox = buttonAABB,
            .getRayBounds = buttonAABB,
            .getCollider = emptyCollider,
        };

        blockBehaviors[BlockType::BLOCK_LEVER] = {
            .getRayBounds = leverAABB,
            .getCollider = emptyCollider,
            // getSelectionBox stays defaultAABB
        };

        blockBehaviors[BlockType::BLOCK_PRESSURE_PLATE_STONE] = {
            .getSelectionBox = pressurePlateAABB,
            .getRayBounds = pressurePlateAABB,
            .getCollider = emptyCollider,
        };
        blockBehaviors[BlockType::BLOCK_PRESSURE_PLATE_WOOD] = {
            .getSelectionBox = pressurePlateAABB,
            .getRayBounds = pressurePlateAABB,
            .getCollider = emptyCollider,
        };

        blockBehaviors[BlockType::BLOCK_TORCH] = {
            .getSelectionBox = torchAABB,
            .getRayBounds = torchAABB,
            .getCollider = emptyCollider,
        };
        blockBehaviors[BlockType::BLOCK_REDSTONE_TORCH_OFF] = {
            .getSelectionBox = torchAABB,
            .getRayBounds = torchAABB,
            .getCollider = emptyCollider,
        };
        blockBehaviors[BlockType::BLOCK_REDSTONE_TORCH_ON] = {
            .getSelectionBox = torchAABB,
            .getRayBounds = torchAABB,
            .getCollider = emptyCollider,
        };

        blockBehaviors[BlockType::BLOCK_PISTON_HEAD] = {
            .getSelectionBox = pistonHeadAABB,
            .getRayBounds = pistonHeadAABB,
            .getCollider = pistonHeadCollider,
        };
    }

} // namespace Blocks