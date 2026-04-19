#pragma once
#include <vector>
#include "world/chunk.h"

namespace DebugGenerator {
    struct BlockEntry {
        uint8_t id;
        std::vector<uint8_t> metaValues;
    };

    static const std::vector<BlockEntry>& getBlockList() {
        static const std::vector<BlockEntry> list = {
            {  1, {0} },                                               // Stone
            {  2, {0} },                                               // Grass
            {  3, {0} },                                               // Dirt
            {  4, {0} },                                               // Cobblestone
            {  5, {0} },                                               // Planks
            {  6, {0,1,2} },                                           // Sapling: oak/spruce/birch
            {  7, {0} },                                               // Bedrock
            {  8, {0,1,2,3,4,5,6,7,8} },                              // Water Flowing: source/levels 1-7/falling
            {  9, {0,1,2,3,4,5,6,7,8} },                              // Water Still: source/levels 1-7/falling
            { 10, {0,2,4,6} },                                       // Lava Flowing: source/levels 1-3/falling
            { 11, {0,2,4,6} },                                       // Lava Still: source/levels 1-3/falling
            { 12, {0} },                                               // Sand
            { 13, {0} },                                               // Gravel
            { 14, {0} },                                               // Gold Ore
            { 15, {0} },                                               // Iron Ore
            { 16, {0} },                                               // Coal Ore
            { 17, {0,1,2} },                                           // Log: oak/spruce/birch
            { 18, {0,1,2} },                                           // Leaves: oak/spruce/birch
            { 19, {0} },                                               // Sponge
            { 20, {0} },                                               // Glass
            { 21, {0} },                                               // Lapis Ore
            { 22, {0} },                                               // Lapis Block
            { 23, {2,3,4,5} },                                         // Dispenser: N/S/W/E
            { 24, {0} },                                               // Sandstone
            { 25, {0} },                                               // Noteblock
            { 26, {0,1,2,3,8,9,10,11} },                               // Bed: dir(0-3) x foot(bit3)
            { 27, {0,1,2,3,4,5,8,9,10,11,12,13} },                    // Powered Rail: shape(0-5) x powered(bit3)
            { 28, {0,1,2,3,4,5,8,9,10,11,12,13} },                    // Detector Rail: shape(0-5) x active(bit3)
            { 29, {0,1,2,3,4,5,8,9,10,11,12,13} },                    // Sticky Piston: facing(0-5) x extended(bit3)
            { 30, {0} },                                               // Cobweb
            { 31, {0,1,2} },                                           // Tall Grass: shrub/grass/fern
            { 32, {0} },                                               // Dead Bush
            { 33, {0,1,2,3,4,5,8,9,10,11,12,13} },                    // Piston: facing(0-5) x extended(bit3)
            { 34, {0,1,2,3,4,5,8,9,10,11,12,13} },                    // Piston Head: facing(0-5) x sticky(bit3)
            { 35, {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} },          // Wool: all 16 colors
            { 37, {0} },                                               // Dandelion
            { 38, {0} },                                               // Rose
            { 39, {0} },                                               // Brown Mushroom
            { 40, {0} },                                               // Red Mushroom
            { 41, {0} },                                               // Gold Block
            { 42, {0} },                                               // Iron Block
            { 43, {0,1,2,3} },                                         // Double Slab: stone/sandstone/wood/cobble
            { 44, {0,1,2,3} },                                         // Slab: stone/sandstone/wood/cobble
            { 45, {0} },                                               // Bricks
            { 46, {0} },                                               // TNT
            { 47, {0} },                                               // Bookshelf
            { 48, {0} },                                               // Mossy Cobblestone
            { 49, {0} },                                               // Obsidian
            { 50, {0,1,2,3,4,5} },                                     // Torch: wall S/N/E/W + floor
            { 51, {0} },                                               // Fire
            { 52, {0} },                                               // Mob Spawner
            { 53, {0,1,2,3} },                                         // Wood Stairs: facing E/W/S/N
            { 54, {0} },                                               // Chest
            { 55, {0,15} },                                            // Redstone Wire: off/full power
            { 56, {0} },                                               // Diamond Ore
            { 57, {0} },                                               // Diamond Block
            { 58, {0} },                                               // Crafting Table
            { 59, {0,1,2,3,4,5,6,7} },                                // Wheat: growth stages 0-7
            { 60, {0,7} },                                             // Farmland: dry/wet
            { 61, {2,3,4,5} },                                         // Furnace: facing N/S/W/E
            { 62, {2,3,4,5} },                                         // Furnace Lit: facing N/S/W/E
            { 64, {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} },          // Wood Door: facing(0-3) x open(bit2) x upper(bit3)
            { 65, {2,3,4,5} },                                         // Ladder: facing N/S/W/E
            { 66, {0,1,2,3,4,5,6,7,8,9} },                            // Rail: all 10 shapes
            { 67, {0,1,2,3} },                                         // Cobblestone Stairs: facing E/W/S/N
            { 69, {1,2,3,4,5,6,9,10,11,12,13,14} },                   // Lever: placement(1-6) x on(bit3)
            { 70, {0,1} },                                             // Stone Pressure Plate: off/on
            { 71, {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} },          // Iron Door: facing(0-3) x open(bit2) x upper(bit3)
            { 72, {0,1} },                                             // Wood Pressure Plate: off/on
            { 73, {0} },                                               // Redstone Ore (off)
            { 74, {0} },                                               // Redstone Ore (glowing)
            { 75, {0,1,2,3,4,5} },                                       // Redstone Torch Off: wall S/N/E/W + floor
            { 76, {0,1,2,3,4,5} },                                       // Redstone Torch On: wall S/N/E/W + floor
            { 77, {1, 2, 3, 4, 9, 10, 11, 12} },                        // Stone Button: facing S/N/E/W unpressed + pressed
            { 78, {0} },                                               // Snow Layer
            { 79, {0} },                                               // Ice
            { 80, {0} },                                               // Snow Block
            { 81, {0} },                                               // Cactus
            { 82, {0} },                                               // Clay
            { 83, {0} },                                               // Sugar Cane
            { 84, {0} },                                               // Jukebox
            { 85, {0} },                                               // Fence
            { 86, {0,1,2,3} },                                         // Pumpkin: facing S/W/N/E
            { 87, {0} },                                               // Netherrack
            { 88, {0} },                                               // Soul Sand
            { 89, {0} },                                               // Glowstone
            { 91, {0,1,2,3} },                                         // Jack-o-Lantern: facing S/W/N/E
            { 92, {0,1,2,3,4,5,6} },                                   // Cake: bites taken 0-6
            { 93, {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} },          // Repeater Off: facing(0-3) x delay(0-3)
            { 94, {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} },          // Repeater On: facing(0-3) x delay(0-3)
            { 96, {0,1,2,3,4,5,6,7} },                                 // Trapdoor: facing(0-3) x open(bit2)
        };
        return list;
    }

    void generateChunk(Chunk& chunk) {
        const auto& blocks = getBlockList();

        // Bedrock floor
        for (int x = 0; x < 16; x++)
            for (int z = 0; z < 16; z++)
                chunk.setBlock({ x, 0, z }, BlockType::BLOCK_STONE);

        if (chunk.cpos.x != 0 || chunk.cpos.z < 0) return;

        const int slotsPerChunk = 16 / 2;  // 8
        const int maxXSlots = 16 / 2;  // 8
        int slotBase = chunk.cpos.z * slotsPerChunk;

        if (slotBase >= (int)blocks.size()) return;

        for (int localSlot = 0; localSlot < slotsPerChunk; localSlot++) {
            int slot = slotBase + localSlot;
            if (slot >= (int)blocks.size()) break;

            const auto& entry = blocks[slot];
            int localZ = localSlot * 2;

            for (int m = 0; m < (int)entry.metaValues.size(); m++) {
                int row = m / maxXSlots;
                int col = m % maxXSlots;
                int localX = col * 2;
                int localY = 1 + row * 2;

                if (localX >= 16) break;

                chunk.setBlock({ localX, localY, localZ }, (BlockType) entry.id);
                chunk.setMeta({ localX, localY, localZ }, entry.metaValues[m]);
            }
        }
    }
}