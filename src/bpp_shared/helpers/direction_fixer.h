/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

// This disgusting code needs to exist because Notch couldn't ever make up his mind about
// which values should indicate the direction of a directional block.
// This is my best attempt at making a somewhat readable and "easy" way
// to normalize that bullshit and put it into a consistent format.
// Please, reuse this code as you need it, or improve it. Nobody should need to do this again!
// 
// This has been made a lot less painful via the Beta Wiki, woohoo!
// http://localhost:5173/beta-wiki/general/blocks#direction-look-up
// 
// Kind regards, Pixel Brush

#pragma once
#include "../numeric_structs.h"
#include "../enums/blocks.h"
#include <array>

// Makes this a little more readable-ish?
enum DirectionBlockType : size_t {
    Torch,
    Lever,
    Button,
    Stairs,
    Doors,
    DispenserFurnaceLadderWallSign,
    Pistons,
    Bed,
    PumpkinJackOLatern,
    Trapdoor,
    RedstoneRepeater,
    MAX_DIRECTION_BLOCK_TYPE
};

// Maps the metadata value to a direction value
static constexpr Direction META_TO_DIRECTION_LUT[MAX_DIRECTION_BLOCK_TYPE][0b111] {
    // Torches
    {Direction::None,Direction::West,Direction::East, Direction::South, Direction::North, Direction::Up, Direction::None},
    // Levers
    {Direction::None,Direction::West,Direction::East, Direction::South, Direction::North, Direction::Up, Direction::Up},
    // Buttons
    {Direction::None,Direction::West,Direction::East, Direction::South, Direction::North, Direction::None, Direction::None},
    // Stairs
    {Direction::East,Direction::West,Direction::South, Direction::North, Direction::None, Direction::None, Direction::None},
    // Doors
    {Direction::East,Direction::South,Direction::West, Direction::North, Direction::None, Direction::None, Direction::None},
    // Dispenser/Furnace/Ladder/Wall Sign
    {Direction::None,Direction::None,Direction::North, Direction::South, Direction::West, Direction::East, Direction::None},
    // Pistons
    {Direction::Down,Direction::Up,Direction::North, Direction::South, Direction::East, Direction::West, Direction::None},
    // Beds
    {Direction::South,Direction::East,Direction::North, Direction::West, Direction::None, Direction::None, Direction::None},
    // Pumpkins/Jack'o'Lantern
    {Direction::South,Direction::West,Direction::North, Direction::East, Direction::None, Direction::None, Direction::None},
    // Trapdoors
    {Direction::South,Direction::North,Direction::East, Direction::West, Direction::None, Direction::None, Direction::None},
    // Redstone Repeater
    {Direction::North,Direction::East,Direction::South, Direction::West, Direction::None, Direction::None, Direction::None},
};

/**
 * @brief Turn a metadata value into an appropriate direction for the block
 * 
 * @param _type Block type
 * @param _meta Metadata value
 * @return Direction 
 */
Direction GetDirectionFromMeta(BlockType _type, uint8_t _meta) {
    switch(_type) {
        case BLOCK_TORCH:
        case BLOCK_REDSTONE_TORCH_OFF:
        case BLOCK_ORE_REDSTONE_ON:
            return META_TO_DIRECTION_LUT[DirectionBlockType::Torch][_meta & 0b11];
        case BLOCK_LEVER:
            // Note: Does not differentiate North-South/East-West direction
            return META_TO_DIRECTION_LUT[DirectionBlockType::Lever][_meta & 0b111];
        case BLOCK_BUTTON_STONE:
            return META_TO_DIRECTION_LUT[DirectionBlockType::Button][_meta & 0b11];
        case BLOCK_STAIRS_COBBLESTONE:
        case BLOCK_STAIRS_WOOD:
            return META_TO_DIRECTION_LUT[DirectionBlockType::Stairs][_meta & 0b11];
        case BLOCK_DOOR_IRON:
        case BLOCK_DOOR_WOOD:
            return META_TO_DIRECTION_LUT[DirectionBlockType::Doors][_meta & 0b11];
        case BLOCK_DISPENSER:
        case BLOCK_FURNACE:
        case BLOCK_FURNACE_LIT:
        case BLOCK_LADDER:
        case BLOCK_SIGN_WALL:
            return META_TO_DIRECTION_LUT[DirectionBlockType::DispenserFurnaceLadderWallSign][_meta & 0b111];
        case BLOCK_PISTON:
        case BLOCK_PISTON_STICKY:
        case BLOCK_PISTON_HEAD:
        // TODO: Check if piston moving uses this too?
            return META_TO_DIRECTION_LUT[DirectionBlockType::Pistons][_meta & 0b111];
        case BLOCK_BED:
            return META_TO_DIRECTION_LUT[DirectionBlockType::Bed][_meta & 0b11];
        case BLOCK_PUMPKIN:
        case BLOCK_PUMPKIN_LIT:
            return META_TO_DIRECTION_LUT[DirectionBlockType::PumpkinJackOLatern][_meta & 0b11];
        case BLOCK_TRAPDOOR:
            return META_TO_DIRECTION_LUT[DirectionBlockType::Trapdoor][_meta & 0b11];
        case BLOCK_REDSTONE_REPEATER_OFF:
        case BLOCK_REDSTONE_REPEATER_ON:
            return META_TO_DIRECTION_LUT[DirectionBlockType::RedstoneRepeater][_meta & 0b11];
        default: // Is a non-directional block
            return Direction::None;
    }
    // TODO: Should probably throw a warning or error?
    return Direction::None;
}

/**
 * @brief Turn a direction into an appropriate metadata value for the block
 * 
 * @param _type Block type
 * @param _dir Direction
 * @return Metadata value 
 */
static constexpr uint8_t GetMetaFromDirection(BlockType _type, Direction _dir) {
    switch(_type) {
        case BLOCK_TORCH:
        case BLOCK_REDSTONE_TORCH_OFF:
        case BLOCK_ORE_REDSTONE_ON:
	    case BLOCK_LEVER:
            switch(_dir) {
		    case Direction::North:
                return 4;
		    case Direction::South:
                return 3;
		    case Direction::East:
                return 2;
		    case Direction::West:
                return 1;
            // Note: For levers, handle North-South/East-West direction separately, defaults to North-South!
		    case Direction::Up:
                return 5;
            default:
                return 0;
		    }
        case BLOCK_BUTTON_STONE:
            switch(_dir) {
		    case Direction::North:
                return 4;
		    case Direction::South:
                return 3;
		    case Direction::East:
                return 2;
		    case Direction::West:
                return 1;
            default:
                return 0;
            }
        case BLOCK_STAIRS_COBBLESTONE:
        case BLOCK_STAIRS_WOOD:
            switch(_dir) {
		    case Direction::North:
                return 3;
		    case Direction::South:
                return 2;
		    case Direction::East:
                return 0;
		    case Direction::West:
                return 1;
            default:
                return 0;
            }
        case BLOCK_DOOR_IRON:
        case BLOCK_DOOR_WOOD:
            switch(_dir) {
		    case Direction::North:
                return 3;
		    case Direction::South:
                return 1;
		    case Direction::East:
                return 0;
		    case Direction::West:
                return 2;
            default:
                return 0;
            }
        case BLOCK_DISPENSER:
        case BLOCK_FURNACE:
        case BLOCK_FURNACE_LIT:
        case BLOCK_LADDER:
        case BLOCK_SIGN_WALL:
            switch(_dir) {
		    case Direction::North:
                return 2;
		    case Direction::South:
                return 3;
		    case Direction::East:
                return 5;
		    case Direction::West:
                return 4;
            default:
                return 0;
            }
        case BLOCK_PISTON:
        case BLOCK_PISTON_STICKY:
        case BLOCK_PISTON_HEAD:
        // TODO: Check if piston moving uses this too?
            switch(_dir) {
		    case Direction::North:
                return 2;
		    case Direction::South:
                return 3;
		    case Direction::East:
                return 4;
		    case Direction::West:
                return 5;
            case Direction::Up:
                return 1;
            case Direction::Down:
                return 0;
            default:
                return 0;
            }
        case BLOCK_BED:
            switch(_dir) {
		    case Direction::North:
                return 2;
		    case Direction::South:
                return 0;
		    case Direction::East:
                return 1;
		    case Direction::West:
                return 3;
            default:
                return 0;
            }
        case BLOCK_PUMPKIN:
        case BLOCK_PUMPKIN_LIT:
            switch(_dir) {
		    case Direction::North:
                return 2;
		    case Direction::South:
                return 0;
		    case Direction::East:
                return 3;
		    case Direction::West:
                return 1;
            default:
                return 0;
            }
        case BLOCK_TRAPDOOR:
            switch(_dir) {
		    case Direction::North:
                return 1;
		    case Direction::South:
                return 0;
		    case Direction::East:
                return 2;
		    case Direction::West:
                return 3;
            default:
                return 0;
            }
        case BLOCK_REDSTONE_REPEATER_OFF:
        case BLOCK_REDSTONE_REPEATER_ON:
            switch(_dir) {
		    case Direction::North:
                return 0;
		    case Direction::South:
                return 2;
		    case Direction::East:
                return 1;
		    case Direction::West:
                return 3;
            default:
                return 0;
            }
        default: // Block without directional data
            return 0;
    }
    // TODO: Should probably throw a warning or error?
    return 0;
}