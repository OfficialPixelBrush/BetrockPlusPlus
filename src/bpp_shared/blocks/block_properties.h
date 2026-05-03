/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include <numeric_structs.h>
#include "enums/blocks.h"
#include "materials.h"
#include "helpers/java/java_random.h"
#include "helpers/AABB.h"

struct WorldManager;
struct Entity;
struct PlayerSession;

namespace Blocks {

    enum class StepSound : uint8_t {
        Stone,   // default, also metal (different pitch)
        Wood,
        Gravel,
        Grass,
        Sand,
        Cloth,
        Glass,
    };

    struct BlockProperties {
        Material  material = Material::Rock();
        StepSound stepSound = StepSound::Stone;

        uint8_t   lightEmission = 0;      // 0-15
        uint8_t   lightOpacity = 255;    // 0 = transparent, 255 = fully opaque
        int       tickRate = 10;

        float     hardness = 1.0f;   // -1 = unbreakable (bedrock)
        float     resistance = 5.0f;   // blast resistance
        float     slipperiness = 0.6f;   // default friction, ice = 0.98f
        float     particleGravity = 1.0f;   // how fast break particles fall

        bool      isCollidable = true;
        bool      isOpaqueCube = true;
        bool      isNormalCube = true;
        bool      renderAsNormalBlock = true;
        bool      ticksOnLoad = false;
        bool      canBlockGrass = true;
        bool      notifyNeighborsOnMetaChange = true;
        bool      enableStats = true; // false = breaking doesn't count for achievements
    };

    struct BlockBehavior {
        // Called when we need to get the AABB for the selection box
        AABB (*getSelectionBox)(uint8_t metadata) = nullptr;

        // Called when we need to check for ray intersections for selection
        AABB (*getRayBounds)(uint8_t metadata) = nullptr;

        // Called when we need to check the collision of this block
        CollisionShape (*getCollider)(uint8_t metadata) = nullptr;

        // Called each random tick if ticksOnLoad = true
        void (*onTick)(WorldManager&, Int3, uint8_t meta, Java::Random&);

        // Called when block is placed by world gen or setBlock
        void (*onBlockAdded)(WorldManager&, Int3) = nullptr;

        // Called when block is removed
        void (*onBlockRemoval)(WorldManager&, Int3) = nullptr;

        // Called when a neighboring block changes
        void (*onNeighborBlockChange)(WorldManager&, Int3, BlockType) = nullptr;

        // Called when a player left-clicks the block (not breaks, just clicks)
        void (*onBlockClicked)(WorldManager&, Int3, uint8_t) = nullptr;

        // Called when a player right-clicks the block — returns true if consumed.
        // PlayerSession is passed so GUI-opening blocks (chests, furnaces, etc.)
        // can send packets directly to the interacting player.
        bool (*onBlockActivated)(WorldManager&, Int3, uint8_t, PlayerSession&) = nullptr;

        // Called when the block is placed by a player or dispenser
        void (*onBlockPlacedBy)(WorldManager&, Int3, uint8_t) = nullptr;

        // Called when block is placed, receives the face it was placed against
        void (*onBlockPlaced)(WorldManager&, Int3, int) = nullptr;

        // Called when player breaks the block
        void (*onBlockDestroyedByPlayer)(WorldManager&, Int3, uint8_t) = nullptr;

        // Called when an explosion destroys the block
        void (*onBlockDestroyedByExplosion)(WorldManager&, Int3) = nullptr;

        // Called when an entity walks on top of the block
        void (*onEntityWalking)(WorldManager&, Int3, Entity&) = nullptr;

        // Called when an entity collides with the block (cactus damage, etc.)
        void (*onEntityCollidedWithBlock)(WorldManager&, Int3, Entity&) = nullptr;

        // Modify entity velocity
        void (*velocityToAddToEntity)(WorldManager&, Int3, Entity&, Vec3&) = nullptr;

        // What item/block this drops when broken
        uint8_t (*idDropped)(uint8_t meta, Java::Random&);

        // How many items drop
        int (*quantityDropped)(Java::Random&);
    };

    // Global tables — indexed by block ID, populated by registerAll()
    extern BlockProperties blockProperties[256];
    extern BlockBehavior   blockBehaviors[256];

    // Call once at startup before anything reads from the tables
    void registerAll();

} // namespace Blocks