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

    void registerAll() {
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
            .isSolid = false,
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
            .lightEmission = 15,    // setLightValue(1.0f) → 15*1.0 = 15
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
            .material = Material::Circuits(),  // MaterialLogic
            .stepSound = StepSound::Stone,      // soundMetalFootstep = Stone pitch 1.5
            .lightOpacity = 0,
            .hardness = 0.7f,
            .isSolid = false,
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
            .isSolid = false,
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
            .isSolid = false,
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
            .isSolid = false,
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
            .hardness = -1.0f,      // unbreakable
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
            .isSolid = false,
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
            .isSolid = false,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Brown Mushroom
        blockProperties[BlockType::BLOCK_MUSHROOM_BROWN] = {
            .material = Material::Plants(),
            .stepSound = StepSound::Grass,
            .lightEmission = 1,     // setLightValue(2.0/16.0) → (int)(15 * 0.125) = 1
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isSolid = false,
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
            .isSolid = false,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
        };

        // Gold Block
        blockProperties[BlockType::BLOCK_GOLD] = {
            .material = Material::Iron(),   // soundMetalFootstep uses Material::iron in source
            .stepSound = StepSound::Stone,   // soundMetalFootstep = Stone at pitch 1.5
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
        // BlockTorch: Material::Circuits, not opaque, not collidable, not solid
        blockProperties[BlockType::BLOCK_TORCH] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Wood,
            .lightEmission = 14,     // setLightValue(15.0/16.0) → (int)(15*0.9375) = 14
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isSolid = false,
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
            .isSolid = false,
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
            .stepSound = StepSound::Stone,   // soundMetalFootstep
            .lightOpacity = 255,
            .hardness = 5.0f,
            .enableStats = false,
        };

        // Oak Wood Stairs
        // BlockStairs copies hardness/resistance/stepSound from planks; isOpaqueCube=false
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
            .stepSound = StepSound::Stone,   // soundPowderFootstep = Stone
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isSolid = false,
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
            .stepSound = StepSound::Stone,   // soundMetalFootstep
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
            .isSolid = false,
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
            .lightEmission = 13,    // setLightValue(14.0/16.0) → (int)(15*0.875) = 13
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
            .isSolid = false,
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
            .isSolid = false,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Rail (normal)
        blockProperties[BlockType::BLOCK_RAIL] = {
            .material = Material::Circuits(),
            .stepSound = StepSound::Stone,   // soundMetalFootstep
            .lightOpacity = 0,
            .hardness = 0.7f,
            .isSolid = false,
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
            .isSolid = false,
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
            .isSolid = false,
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
            .isSolid = false,
            .isCollidable = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .notifyNeighborsOnMetaChange = false,
        };

        // Iron Door
        blockProperties[BlockType::BLOCK_DOOR_IRON] = {
            .material = Material::Iron(),
            .stepSound = StepSound::Stone,   // soundMetalFootstep
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
            .isSolid = false,
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
            .lightEmission = 9,    // setLightValue(10.0/16.0) → (int)(15*0.625) = 9
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
            .isSolid = false,
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
            .lightEmission = 7,    // setLightValue(0.5) → (int)(15*0.5) = 7
            .lightOpacity = 0,
            .hardness = 0.0f,
            .isSolid = false,
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
            .isSolid = false,
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
            .isSolid = false,
            .isOpaqueCube = false,
            .isNormalCube = false,
            .renderAsNormalBlock = false,
            .canBlockGrass = false,
        };

        // Ice
        // slipperiness = 0.98 (set in BlockIce constructor)
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
            .isSolid = false,
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
            .lightOpacity = 255,
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

        // Slows entities: velocityToAddToEntity multiplies motionX/Z by 0.4
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
        // BlockBreakable, Material::Portal, not opaque, unbreakable (hardness -1)
        blockProperties[BlockType::BLOCK_NETHER_PORTAL] = {
            .material = Material::Portal(),
            .stepSound = StepSound::Glass,
            .lightEmission = 11,    // setLightValue(12.0/16.0) → (int)(15*0.75) = 11
            .lightOpacity = 0,
            .hardness = -1.0f,
            .isSolid = false,
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
            .lightEmission = 9,    // setLightValue(10.0/16.0) → (int)(15*0.625) = 9
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
    }

} // namespace Blocks