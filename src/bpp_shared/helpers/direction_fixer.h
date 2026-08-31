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
#include "../enums/blocks.h"
#include "../enums/network/packet_data.h"
#include "../numeric_structs.h"
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
static constexpr Direction::Value META_TO_DIRECTION_LUT[MAX_DIRECTION_BLOCK_TYPE][0b111]{
	// Torches
	{ Direction::Value::None, Direction::Value::East, Direction::Value::West, Direction::Value::South,
	  Direction::Value::North, Direction::Value::Up, Direction::Value::None },
	// Levers
	{ Direction::Value::None, Direction::Value::East, Direction::Value::West, Direction::Value::South,
	  Direction::Value::North, Direction::Value::Up, Direction::Value::Up },
	// Buttons
	{ Direction::Value::None, Direction::Value::East, Direction::Value::West, Direction::Value::South,
	  Direction::Value::North, Direction::Value::None, Direction::Value::None },
	// Stairs
	{ Direction::Value::East, Direction::Value::West, Direction::Value::South, Direction::Value::North,
	  Direction::Value::None, Direction::Value::None, Direction::Value::None },
	// Doors
	{ Direction::Value::East, Direction::Value::South, Direction::Value::West, Direction::Value::North,
	  Direction::Value::None, Direction::Value::None, Direction::Value::None },
	// Dispenser/Furnace/Ladder/Wall Sign
	{ Direction::Value::None, Direction::Value::None, Direction::Value::North, Direction::Value::South,
	  Direction::Value::West, Direction::Value::East, Direction::Value::None },
	// Pistons
	{ Direction::Value::Down, Direction::Value::Up, Direction::Value::North, Direction::Value::South,
	  Direction::Value::East, Direction::Value::West, Direction::Value::None },
	// Beds
	{ Direction::Value::South, Direction::Value::West, Direction::Value::North, Direction::Value::East,
	  Direction::Value::None, Direction::Value::None, Direction::Value::None },
	// Pumpkins/Jack'o'Lantern
	{ Direction::Value::South, Direction::Value::West, Direction::Value::North, Direction::Value::East,
	  Direction::Value::None, Direction::Value::None, Direction::Value::None },
	// Trapdoors
	{ Direction::Value::South, Direction::Value::North, Direction::Value::East, Direction::Value::West,
	  Direction::Value::None, Direction::Value::None, Direction::Value::None },
	// Redstone Repeater
	{ Direction::Value::North, Direction::Value::East, Direction::Value::South, Direction::Value::West,
	  Direction::Value::None, Direction::Value::None, Direction::Value::None },
};

/**
 * @brief Turn a metadata value into an appropriate direction for the block
 * 
 * @param _type Block type
 * @param _meta Metadata value
 * @return Direction::Value 
 */
static constexpr Direction::Value GetDirectionFromMeta(const BlockType _type, const uint8_t _meta) {
	switch (_type) {
	case BLOCK_TORCH:
	case BLOCK_REDSTONE_TORCH_OFF:
	case BLOCK_REDSTONE_TORCH_ON:
		return META_TO_DIRECTION_LUT[DirectionBlockType::Torch][_meta & 0b111];
	case BLOCK_LEVER:
		// Note: Does not differentiate North-South/East-West direction
		return META_TO_DIRECTION_LUT[DirectionBlockType::Lever][_meta & 0b111];
	case BLOCK_BUTTON_STONE:
		return META_TO_DIRECTION_LUT[DirectionBlockType::Button][_meta & 0b111];
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
		return Direction::Value::None;
	}
	// TODO: Should probably throw a warning or error?
	return Direction::Value::None;
}

/**
 * @brief Turn a direction into an appropriate metadata value for the block
 * 
 * @param _type Block type
 * @param _dir Direction::Value
 * @return Metadata value 
 */
static constexpr uint8_t GetMetaFromDirection(const BlockType _type, const Direction::Value _dir) {
	switch (_type) {
	case BLOCK_TORCH:
	case BLOCK_REDSTONE_TORCH_OFF:
	case BLOCK_REDSTONE_TORCH_ON:
	case BLOCK_LEVER:
		switch (_dir) {
		case Direction::Value::North:
			return 4;
		case Direction::Value::South:
			return 3;
		case Direction::Value::East:
			return 1;
		case Direction::Value::West:
			return 2;
		// Note: For levers, handle North-South/East-West direction separately, defaults to North-South!
		case Direction::Value::Up:
			return 5;
		default:
			return 0;
		}
	case BLOCK_BUTTON_STONE:
		switch (_dir) {
		case Direction::Value::North:
			return 4;
		case Direction::Value::South:
			return 3;
		case Direction::Value::East:
			return 1;
		case Direction::Value::West:
			return 2;
		default:
			return 0;
		}
	case BLOCK_STAIRS_COBBLESTONE:
	case BLOCK_STAIRS_WOOD:
		switch (_dir) {
		case Direction::Value::North:
			return 3;
		case Direction::Value::South:
			return 2;
		case Direction::Value::East:
			return 0;
		case Direction::Value::West:
			return 1;
		default:
			return 0;
		}
	case BLOCK_DOOR_IRON:
	case BLOCK_DOOR_WOOD:
		switch (_dir) {
		case Direction::Value::North:
			return 3;
		case Direction::Value::South:
			return 1;
		case Direction::Value::East:
			return 0;
		case Direction::Value::West:
			return 2;
		default:
			return 0;
		}
	case BLOCK_DISPENSER:
	case BLOCK_FURNACE:
	case BLOCK_FURNACE_LIT:
	case BLOCK_LADDER:
	case BLOCK_SIGN_WALL:
		switch (_dir) {
		case Direction::Value::North:
			return 2;
		case Direction::Value::South:
			return 3;
		case Direction::Value::East:
			return 5;
		case Direction::Value::West:
			return 4;
		default:
			return 0;
		}
	case BLOCK_PISTON:
	case BLOCK_PISTON_STICKY:
	case BLOCK_PISTON_HEAD:
		// TODO: Check if piston moving uses this too?
		switch (_dir) {
		case Direction::Value::North:
			return 2;
		case Direction::Value::South:
			return 3;
		case Direction::Value::East:
			return 4;
		case Direction::Value::West:
			return 5;
		case Direction::Value::Up:
			return 1;
		case Direction::Value::Down:
			return 0;
		default:
			return 0;
		}
	case BLOCK_BED:
		switch (_dir) {
		case Direction::Value::North:
			return 2;
		case Direction::Value::South:
			return 0;
		case Direction::Value::East:
			return 3;
		case Direction::Value::West:
			return 1;
		default:
			return 0;
		}
	case BLOCK_PUMPKIN:
	case BLOCK_PUMPKIN_LIT:
		switch (_dir) {
		case Direction::Value::North:
			return 2;
		case Direction::Value::South:
			return 0;
		case Direction::Value::East:
			return 3;
		case Direction::Value::West:
			return 1;
		default:
			return 0;
		}
	case BLOCK_TRAPDOOR:
		switch (_dir) {
		case Direction::Value::North:
			return 1;
		case Direction::Value::South:
			return 0;
		case Direction::Value::East:
			return 2;
		case Direction::Value::West:
			return 3;
		default:
			return 0;
		}
	case BLOCK_REDSTONE_REPEATER_OFF:
	case BLOCK_REDSTONE_REPEATER_ON:
		switch (_dir) {
		case Direction::Value::North:
			return 0;
		case Direction::Value::South:
			return 2;
		case Direction::Value::East:
			return 1;
		case Direction::Value::West:
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

static constexpr Direction::Value FaceDirectionToDirection(const PacketData::FaceDirection _face) {
	switch (_face) {
	case PacketData::Z_MINUS:
		return Direction::Value::North;
	case PacketData::Z_PLUS:
		return Direction::Value::South;
	case PacketData::X_MINUS:
		return Direction::Value::West;
	case PacketData::X_PLUS:
		return Direction::Value::East;
	case PacketData::Y_MINUS:
		return Direction::Value::Down;
	case PacketData::Y_PLUS:
		return Direction::Value::Up;
	default:
		return Direction::Value::None;
	}
}

static constexpr PacketData::FaceDirection DirectionToFaceDirection(const Direction::Value _dir) {
	switch (_dir) {
	case Direction::Value::North:
		return PacketData::Z_MINUS;
	case Direction::Value::South:
		return PacketData::Z_PLUS;
	case Direction::Value::West:
		return PacketData::X_MINUS;
	case Direction::Value::East:
		return PacketData::X_PLUS;
	case Direction::Value::Down:
		return PacketData::Y_MINUS;
	case Direction::Value::Up:
		return PacketData::Y_PLUS;
	default:
		return PacketData::INVALID_USE;
	}
}