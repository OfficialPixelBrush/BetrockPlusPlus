/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "rail_manager.h"
#include "redstone_manager.h"
#include "world.h"
#include <algorithm>

Int3 RailManager::FindRailConnection(WorldManager& _world, Int3 _pos, Direction::Value _dir) {
	// Order here matters!
	Int3 base = _pos.WithOffset(_dir);
	Direction::Value offsets[3] = { Direction::Value::None, Direction::Value::Up, Direction::Value::Down };
	for (size_t i = 0; i < 3; i++) {
		Int3 checkPos = base.WithOffset(offsets[i]);
		if (RailManager::IsRail(_world.GetBlockId(checkPos)))
			return checkPos;
	}
	return _pos; // Return our own pos if nothing is found
}

// TODO: Could this be turned into an array too?
std::vector<Direction::Value> RailManager::GetVerifiedConnections(WorldManager& _world, Int3 _pos) {
	auto shape = GetRailShape(_world.GetMetadata(_pos), _world.GetBlockId(_pos));
	auto implied = GetImpliedConnections(shape);

	std::vector<Direction::Value> verified;
	for (auto dir : implied) {
		Int3 farPos = FindRailConnection(_world, _pos, dir);
		if (farPos == _pos)
			continue; // nothing actually there

		// Does the far rail's OWN shape point back at _pos?
		auto farShape = GetRailShape(_world.GetMetadata(farPos), _world.GetBlockId(farPos));
		bool pointsBack = false;
		for (auto farDir : GetImpliedConnections(farShape)) {
			if (FindRailConnection(_world, farPos, farDir) == _pos) {
				pointsBack = true;
				break;
			}
		}
		if (pointsBack)
			verified.push_back(dir);
	}
	return verified;
}

static Direction::Value GetCheckDir(Int3 _pos, Int3 _connectionPos) {
	Int3 connectionPosModified = _connectionPos;
	connectionPosModified.y = _pos.y;
	auto d = connectionPosModified - _pos;

	// This is cancerous
	Vec3 deltaDouble = { double(d.x), double(d.y), double(d.z) };
	auto deltaDir = deltaDouble.GetFacingDirections()[0];
	return Direction::Opposite(deltaDir);
}

bool RailManager::CanConnect(WorldManager& _world, Int3 _pos, Direction::Value _dir) {
	Int3 connectionPos = FindRailConnection(_world, _pos, _dir);
	if (connectionPos == _pos)
		return false;

	auto neighborConnections = GetVerifiedConnections(_world, connectionPos);
	auto checkDir = GetCheckDir(_pos, connectionPos);

	// Is our neighbor already pointing to us?
	for (auto& dir : neighborConnections) {
		if (dir == checkDir)
			return true;
	}
	return neighborConnections.size() < 2;
}

bool RailManager::IsActuallyConnected(WorldManager& _world, Int3 _pos, Direction::Value _dir) {
	Int3 connectionPos = FindRailConnection(_world, _pos, _dir);
	if (connectionPos == _pos)
		return false;

	auto checkDir = GetCheckDir(_pos, connectionPos);
	for (auto& dir : GetVerifiedConnections(_world, connectionPos)) {
		if (dir == checkDir)
			return true;
	}
	return false;
}

int RailManager::GetAdjacentTrackCount(WorldManager& _world, Int3 _pos) {
	int count = 0;
	for (auto dir :
	     { Direction::Value::North, Direction::Value::South, Direction::Value::East, Direction::Value::West }) {
		if (FindRailConnection(_world, _pos, dir) != _pos)
			count++;
	}
	return count;
}

static Blocks::RailShape ResolveCleanShape(bool _cn, bool _cs, bool _ce, bool _cw, bool _canCurve) {
	Blocks::RailShape shape = Blocks::RailShape::INVALID;
	bool connectedOnZ = _cn || _cs;
	bool connectedOnX = _ce || _cw;

	if (connectedOnZ && !connectedOnX)
		shape = Blocks::RailShape::FlatNorthSouth;
	if (connectedOnX && !connectedOnZ)
		shape = Blocks::RailShape::FlatEastWest;

	if (_canCurve) {
		if (_cs && _ce && !_cn && !_cw)
			shape = Blocks::RailShape::CurveSouthEast;
		if (_cs && _cw && !_cn && !_ce)
			shape = Blocks::RailShape::CurveSouthWest;
		if (_cn && _ce && !_cs && !_cw)
			shape = Blocks::RailShape::CurveNorthEast;
		if (_cn && _cw && !_cs && !_ce)
			shape = Blocks::RailShape::CurveNorthWest;
	}
	return shape;
}

static Blocks::RailShape ApplySlope(WorldManager& _world, Int3 _pos, Blocks::RailShape _shape) {
	if (_shape == Blocks::RailShape::FlatNorthSouth) {
		if (RailManager::IsRail(
		        _world.GetBlockId(_pos.WithOffset(Direction::Value::North).WithOffset(Direction::Value::Up))))
			_shape = Blocks::RailShape::AscendingNorth;
		if (RailManager::IsRail(
		        _world.GetBlockId(_pos.WithOffset(Direction::Value::South).WithOffset(Direction::Value::Up))))
			_shape = Blocks::RailShape::AscendingSouth;
	}
	if (_shape == Blocks::RailShape::FlatEastWest) {
		if (RailManager::IsRail(
		        _world.GetBlockId(_pos.WithOffset(Direction::Value::East).WithOffset(Direction::Value::Up))))
			_shape = Blocks::RailShape::AscendingEast;
		if (RailManager::IsRail(
		        _world.GetBlockId(_pos.WithOffset(Direction::Value::West).WithOffset(Direction::Value::Up))))
			_shape = Blocks::RailShape::AscendingWest;
	}
	return _shape;
}

Blocks::RailShape RailManager::DetermineRailShape(WorldManager& _world, Int3 _pos, BlockType _block) {
	bool canCurve = RailManager::RailCanCurve(_block);

	auto cn = CanConnect(_world, _pos, Direction::Value::North);
	auto cs = CanConnect(_world, _pos, Direction::Value::South);
	auto ce = CanConnect(_world, _pos, Direction::Value::East);
	auto cw = CanConnect(_world, _pos, Direction::Value::West);

	Blocks::RailShape shape = ResolveCleanShape(cn, cs, ce, cw, canCurve);

	if (shape == Blocks::RailShape::INVALID) {
		if (cn || cs)
			shape = Blocks::RailShape::FlatNorthSouth;
		if (ce || cw)
			shape = Blocks::RailShape::FlatEastWest;

		if (canCurve) {
			// This is why powering a rail flips it!
			bool poweredHere = RedstoneManager::IsPositionPowered(_world, _pos);
			if (poweredHere) {
				if (cs && ce)
					shape = Blocks::RailShape::CurveSouthEast;
				if (cs && cw)
					shape = Blocks::RailShape::CurveSouthWest;
				if (cn && ce)
					shape = Blocks::RailShape::CurveNorthEast;
				if (cn && cw)
					shape = Blocks::RailShape::CurveNorthWest;
			} else {
				if (cn && cw)
					shape = Blocks::RailShape::CurveNorthWest;
				if (cn && ce)
					shape = Blocks::RailShape::CurveNorthEast;
				if (cs && cw)
					shape = Blocks::RailShape::CurveSouthWest;
				if (cs && ce)
					shape = Blocks::RailShape::CurveSouthEast;
			}
		}
	}

	shape = ApplySlope(_world, _pos, shape);
	return shape == Blocks::RailShape::INVALID ? Blocks::RailShape::FlatNorthSouth : shape;
}

Blocks::RailShape RailManager::DetermineRailShapeWithAddition(WorldManager& _world, Int3 _pos, BlockType _block,
                                                              Direction::Value _newDir) {
	bool cn = false, cs = false, ce = false, cw = false;
	auto mark = [&](Direction::Value _dir) {
		switch (_dir) {
		case Direction::Value::North:
			cn = true;
			break;
		case Direction::Value::South:
			cs = true;
			break;
		case Direction::Value::East:
			ce = true;
			break;
		case Direction::Value::West:
			cw = true;
			break;
		default:
			break;
		}
	};

	for (auto dir : GetVerifiedConnections(_world, _pos))
		mark(dir);
	mark(_newDir);

	bool canCurve = RailManager::RailCanCurve(_block);
	Blocks::RailShape shape = ResolveCleanShape(cn, cs, ce, cw, canCurve);

	if (shape == Blocks::RailShape::INVALID) {
		if (cn || cs)
			shape = Blocks::RailShape::FlatNorthSouth;
		if (ce || cw)
			shape = Blocks::RailShape::FlatEastWest;
	}

	shape = ApplySlope(_world, _pos, shape);
	return shape == Blocks::RailShape::INVALID ? Blocks::RailShape::FlatNorthSouth : shape;
}

bool RailManager::IsRailPowered(WorldManager& _world, Int3 _pos) {
	auto block = _world.GetBlockId(_pos);
	if (block == BLOCK_RAIL_POWERED)
		return _world.GetMetadata(_pos) & 0b1000;
	return false;
}

static bool CanPoweredRailContinuePower(WorldManager& _world, Int3 _pos, Int3 _from, int8_t& _distance) {
	// I love recursion
	if (_distance >= 8)
		return false;

	_distance++;

	if (RedstoneManager::IsPositionPowered(_world, _pos)) {
		// We are directly powered so return here
		return true;
	}

	auto connections = RailManager::GetImpliedConnections(
	    RailManager::GetRailShape(_world.GetMetadata(_pos), BLOCK_RAIL_POWERED));
	for (auto& connection : connections) {
		if (!RailManager::IsActuallyConnected(_world, _pos, connection))
			continue;

		Int3 neighborPos = RailManager::FindRailConnection(_world, _pos, connection);
		if (neighborPos == _pos || neighborPos == _from)
			continue;

		if (!RailManager::IsRailPowered(_world, neighborPos))
			continue;

		if (CanPoweredRailContinuePower(_world, neighborPos, _pos, _distance)) {
			// If this neighbor is directly powered then great! Return here
			return true;
		}
	}

	// We couldn't find any power within 8 blocks :(
	return false;
}

void RailManager::UpdateRailPower(WorldManager& _world, Int3 _pos, BlockType _block) {
	if (_block != BLOCK_RAIL_POWERED)
		return;

	// Check if we are powered directly
	auto meta = _world.GetMetadata(_pos);
	bool currentlyPowered = (meta & 0x8) != 0;
	bool shouldBePowered = RedstoneManager::IsPositionPowered(_world, _pos);

	// Check our neighbor rails
	if (!shouldBePowered) {
		auto connections = GetImpliedConnections(GetRailShape(_world.GetMetadata(_pos), _block));
		for (auto& connection : connections) {
			if (!IsActuallyConnected(_world, _pos, connection))
				continue;

			Int3 neighborPos = FindRailConnection(_world, _pos, connection);
			if (neighborPos == _pos)
				continue;
			if (!IsRailPowered(_world, neighborPos))
				continue;

			if (!RedstoneManager::IsPositionPowered(_world, neighborPos)) {
				// If the neighbor is powered but not by redstone,
				// That means its apart of a longer chain of powered rails
				// So we look up to 8 rails away for a proper source of power
				int8_t distance = 0;
				if (CanPoweredRailContinuePower(_world, neighborPos, _pos, distance)) {
					shouldBePowered = true;
				}
				continue;
			}

			shouldBePowered = true;
			if (shouldBePowered)
				break;
		}
	}

	// Update our power state
	if (shouldBePowered != currentlyPowered) {
		uint8_t newMeta = shouldBePowered ? (meta | 0x8) : (meta & ~0x8);
		_world.SetMeta(_pos, newMeta);
		_world.NotifyNeighborsOfUpdate(_pos.WithOffset(Direction::Value::Down), BLOCK_RAIL_POWERED);
		auto shape = GetRailShape(newMeta, _block);
		if (shape == Blocks::RailShape::AscendingEast || shape == Blocks::RailShape::AscendingWest ||
		    shape == Blocks::RailShape::AscendingNorth || shape == Blocks::RailShape::AscendingSouth) {
			_world.NotifyNeighborsOfUpdate(_pos.WithOffset(Direction::Value::Up), BLOCK_RAIL_POWERED);
		}
	}
}

static uint8_t EncodeMeta(WorldManager& _world, Int3 _pos, BlockType _block, Blocks::RailShape _shape) {
	uint8_t oldMeta = _world.GetMetadata(_pos);
	uint8_t poweredBit = (_block != BLOCK_RAIL) ? (oldMeta & 0x8) : 0;
	return static_cast<uint8_t>(_shape) | poweredBit;
}

void RailManager::RefreshRail(WorldManager& _world, Int3 _pos, BlockType _block, bool _forceWrite) {
	uint8_t oldMeta = _world.GetMetadata(_pos);
	auto newShape = DetermineRailShape(_world, _pos, _block);
	uint8_t newMeta = EncodeMeta(_world, _pos, _block, newShape);

	if (!_forceWrite && newMeta == oldMeta)
		return;

	_world.SetMeta(_pos, newMeta);

	for (auto& dir : GetImpliedConnections(newShape)) {
		Int3 neighborPos = FindRailConnection(_world, _pos, dir);
		if (neighborPos == _pos)
			continue; // nothing actually there

		auto neighborBlock = _world.GetBlockId(neighborPos);
		auto neighborShape = DetermineRailShapeWithAddition(_world, neighborPos, neighborBlock,
		                                                    Direction::Opposite(dir));
		uint8_t neighborMeta = EncodeMeta(_world, neighborPos, neighborBlock, neighborShape);
		_world.SetMeta(neighborPos, neighborMeta);

		if (neighborBlock == BLOCK_RAIL_POWERED)
			UpdateRailPower(_world, neighborPos, neighborBlock);
	}
}