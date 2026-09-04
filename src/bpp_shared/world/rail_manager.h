/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "../helpers/direction_fixer.h"
#include "base_structs.h"
#include "blocks.h"
#include "blocks/block_behaviors.h"
#include "blocks/block_properties_behaviors.h"
#include "blocks/block_builder.h"

struct WorldManager;
struct RailManager {
	// Awe shit, here we go again
	// Rails have a LOT of logic that goes on so
	// Separate file!! (block_behaviors.cpp is already MASSIVE)
	bool IsRail(BlockType _block) {
		return _block == BLOCK_RAIL || _block == BLOCK_RAIL_POWERED || _block == BLOCK_RAIL_DETECTOR;
	}

	bool IsRailPowered(WorldManager& _world, Int3 _pos);
	
	Blocks::RailShape GetRailShape(uint8_t _meta, BlockType _block) {
		if (!IsRail(_block))
			return Blocks::RailShape::FlatNorthSouth;
		// Powered / Detector rails can't curve
		return static_cast<Blocks::RailShape>(_meta & (_block == BLOCK_RAIL ? 0xF : 0x7));
	}
	
	std::vector<Direction::Value> GetImpliedConnections(uint8_t _meta) {
		// Name is a little confusing,
		// But this is just what possible connection points exist
		// Given the metadata of the rail
		auto shape = GetRailShape(_meta, BLOCK_RAIL);
		switch (shape) { 
		case Blocks::RailShape::FlatNorthSouth:
		case Blocks::RailShape::AscendingNorth:
		case Blocks::RailShape::AscendingSouth:
			return { Direction::Value::North, Direction::Value::South };
		case Blocks::RailShape::FlatEastWest:
		case Blocks::RailShape::AscendingEast:
		case Blocks::RailShape::AscendingWest:
			return { Direction::Value::East, Direction::Value::West };
		case Blocks::RailShape::CurveNorthEast:
			return { Direction::Value::North, Direction::Value::East };
		case Blocks::RailShape::CurveSouthEast:
			return { Direction::Value::South, Direction::Value::East };
		case Blocks::RailShape::CurveSouthWest:
			return { Direction::Value::South, Direction::Value::West };
		case Blocks::RailShape::CurveNorthWest:
			return { Direction::Value::North, Direction::Value::West };
		}
	}
};