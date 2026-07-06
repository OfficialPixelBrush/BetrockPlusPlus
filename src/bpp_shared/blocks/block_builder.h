/*
 * Copyright (c) 2026, mudkipdev <github.com/mudkipdev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include "base_structs.h"
#include <algorithm>
#include <cstdint>

namespace Blocks {
enum class Direction : uint8_t {
	North = 0,
	East = 1,
	South = 2,
	West = 3,
};

enum class WoodType : uint8_t {
	Oak = 0,
	Spruce = 1,
	Birch = 2,
};

enum class WoolColor : uint8_t {
	White = 0,
	Orange = 1,
	Magenta = 2,
	LightBlue = 3,
	Yellow = 4,
	Lime = 5,
	Pink = 6,
	Gray = 7,
	LightGray = 8,
	Cyan = 9,
	Purple = 10,
	Blue = 11,
	Brown = 12,
	Green = 13,
	Red = 14,
	Black = 15,
};

enum class SlabType : uint8_t {
	Stone = 0,
	Sandstone = 1,
	Wood = 2,
	Cobblestone = 3,
};

enum class TallGrassType : uint8_t {
	DeadBush = 0,
	TallGrass = 1,
	Fern = 2,
};

enum class TorchAttachment : uint8_t {
	WestWall = 1,
	EastWall = 2,
	NorthWall = 3,
	SouthWall = 4,
	Floor = 5,
};

enum class WallFacing : uint8_t {
	North = 2,
	South = 3,
	West = 4,
	East = 5,
};

enum class LeverMount : uint8_t {
	WestWall = 1,
	EastWall = 2,
	NorthWall = 3,
	SouthWall = 4,
	FloorEastWest = 5,
	FloorNorthSouth = 6,
};

enum class ButtonMount : uint8_t {
	WestWall = 1,
	EastWall = 2,
	NorthWall = 3,
	SouthWall = 4,
};

enum class RailShape : uint8_t {
	FlatNorthSouth = 0,
	FlatEastWest = 1,
	AscendingEast = 2,
	AscendingWest = 3,
	AscendingNorth = 4,
	AscendingSouth = 5,
	CurveNorthEast = 6,
	CurveSouthEast = 7,
	CurveSouthWest = 8,
	CurveNorthWest = 9,
};

enum class PistonFacing : uint8_t {
	Down = 0,
	Up = 1,
	North = 2,
	South = 3,
	West = 4,
	East = 5,
};

enum class StairsDirection : uint8_t {
	East = 0,
	West = 1,
	South = 2,
	North = 3,
};

struct StairsBuilder {
	constexpr explicit StairsBuilder(BlockType id) : m_id(id) {}

	StairsBuilder facing(StairsDirection direction) const {
		StairsBuilder b = *this;
		b.m_direction = direction;
		return b;
	}

	Block asBlock() const {
		return Block{ m_id, static_cast<uint8_t>(m_direction) };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id;
	StairsDirection m_direction = StairsDirection::East;
};

struct FluidBuilder {
	constexpr explicit FluidBuilder(BlockType id) : m_id(id) {}

	FluidBuilder level(uint8_t level) const {
		FluidBuilder b = *this;
		b.m_level = std::min<uint8_t>(level, 7);
		return b;
	}
	FluidBuilder falling(bool falling) const {
		FluidBuilder b = *this;
		b.m_falling = falling;
		return b;
	}

	Block asBlock() const {
		uint8_t data = m_level & 0x7;
		if (m_falling)
			data |= 0x8;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id;
	uint8_t m_level = 0;
	bool m_falling = false;
};

struct SaplingBuilder {
	SaplingBuilder type(WoodType type) const {
		SaplingBuilder b = *this;
		b.m_type = type;
		return b;
	}
	SaplingBuilder readyToGrow(bool readyToGrow) const {
		SaplingBuilder b = *this;
		b.m_readyToGrow = readyToGrow;
		return b;
	}

	Block asBlock() const {
		uint8_t data = static_cast<uint8_t>(m_type);
		if (m_readyToGrow)
			data |= 0x8;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_SAPLING;
	WoodType m_type = WoodType::Oak;
	bool m_readyToGrow = false;
};

struct LogBuilder {
	LogBuilder type(WoodType type) const {
		LogBuilder b = *this;
		b.m_type = type;
		return b;
	}

	Block asBlock() const {
		return Block{ m_id, static_cast<uint8_t>(m_type) };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_LOG;
	WoodType m_type = WoodType::Oak;
};

struct LeavesBuilder {
	LeavesBuilder type(WoodType type) const {
		LeavesBuilder b = *this;
		b.m_type = type;
		return b;
	}
	LeavesBuilder decaying(bool decaying) const {
		LeavesBuilder b = *this;
		b.m_decaying = decaying;
		return b;
	}

	Block asBlock() const {
		uint8_t data = static_cast<uint8_t>(m_type);
		if (m_decaying)
			data |= 0x8;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_LEAVES;
	WoodType m_type = WoodType::Oak;
	bool m_decaying = false;
};

struct TallGrassBuilder {
	TallGrassBuilder type(TallGrassType type) const {
		TallGrassBuilder b = *this;
		b.m_type = type;
		return b;
	}

	Block asBlock() const {
		return Block{ m_id, static_cast<uint8_t>(m_type) };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_TALLGRASS;
	TallGrassType m_type = TallGrassType::TallGrass;
};

struct WoolBuilder {
	WoolBuilder color(WoolColor color) const {
		WoolBuilder b = *this;
		b.m_color = color;
		return b;
	}

	Block asBlock() const {
		return Block{ m_id, static_cast<uint8_t>(m_color) };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_WOOL;
	WoolColor m_color = WoolColor::White;
};

struct SlabBuilder {
	constexpr explicit SlabBuilder(BlockType id) : m_id(id) {}

	SlabBuilder type(SlabType type) const {
		SlabBuilder b = *this;
		b.m_type = type;
		return b;
	}

	Block asBlock() const {
		return Block{ m_id, static_cast<uint8_t>(m_type) };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id;
	SlabType m_type = SlabType::Stone;
};

struct TorchBuilder {
	constexpr explicit TorchBuilder(BlockType id) : m_id(id) {}

	TorchBuilder attachment(TorchAttachment attachment) const {
		TorchBuilder b = *this;
		b.m_attachment = attachment;
		return b;
	}

	Block asBlock() const {
		return Block{ m_id, static_cast<uint8_t>(m_attachment) };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id;
	TorchAttachment m_attachment = TorchAttachment::Floor;
};

struct WallFacingBuilder {
	constexpr explicit WallFacingBuilder(BlockType id) : m_id(id) {}

	WallFacingBuilder facing(WallFacing facing) const {
		WallFacingBuilder b = *this;
		b.m_facing = facing;
		return b;
	}

	Block asBlock() const {
		return Block{ m_id, static_cast<uint8_t>(m_facing) };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id;
	WallFacing m_facing = WallFacing::North;
};

struct BedBuilder {
	BedBuilder facing(Direction direction) const {
		BedBuilder b = *this;
		b.m_direction = direction;
		return b;
	}
	BedBuilder occupied(bool occupied) const {
		BedBuilder b = *this;
		b.m_occupied = occupied;
		return b;
	}
	BedBuilder head(bool head) const {
		BedBuilder b = *this;
		b.m_head = head;
		return b;
	}

	Block asBlock() const {
		uint8_t data;
		switch (m_direction) {
		case Direction::South:
			data = 0;
			break;
		case Direction::West:
			data = 1;
			break;
		case Direction::North:
			data = 2;
			break;
		case Direction::East:
			data = 3;
			break;
		}
		if (m_occupied)
			data |= 0x4;
		if (m_head)
			data |= 0x8;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_BED;
	Direction m_direction = Direction::South;
	bool m_occupied = false;
	bool m_head = false;
};

struct PoweredRailBuilder {
	constexpr explicit PoweredRailBuilder(BlockType id) : m_id(id) {}

	PoweredRailBuilder shape(RailShape shape) const {
		PoweredRailBuilder b = *this;
		b.m_shape = shape;
		return b;
	}
	PoweredRailBuilder powered(bool powered) const {
		PoweredRailBuilder b = *this;
		b.m_powered = powered;
		return b;
	}

	Block asBlock() const {
		uint8_t data = static_cast<uint8_t>(m_shape) & 0x7;
		if (m_powered)
			data |= 0x8;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id;
	RailShape m_shape = RailShape::FlatNorthSouth;
	bool m_powered = false;
};

struct PistonBuilder {
	constexpr explicit PistonBuilder(BlockType id) : m_id(id) {}

	PistonBuilder facing(PistonFacing facing) const {
		PistonBuilder b = *this;
		b.m_facing = facing;
		return b;
	}
	PistonBuilder extended(bool extended) const {
		PistonBuilder b = *this;
		b.m_extended = extended;
		return b;
	}

	Block asBlock() const {
		uint8_t data = static_cast<uint8_t>(m_facing);
		if (m_extended)
			data |= 0x8;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id;
	PistonFacing m_facing = PistonFacing::Up;
	bool m_extended = false;
};

struct PistonHeadBuilder {
	PistonHeadBuilder facing(PistonFacing facing) const {
		PistonHeadBuilder b = *this;
		b.m_facing = facing;
		return b;
	}
	PistonHeadBuilder sticky(bool sticky) const {
		PistonHeadBuilder b = *this;
		b.m_sticky = sticky;
		return b;
	}

	Block asBlock() const {
		uint8_t data = static_cast<uint8_t>(m_facing);
		if (m_sticky)
			data |= 0x8;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_PISTON_HEAD;
	PistonFacing m_facing = PistonFacing::Up;
	bool m_sticky = false;
};

struct DoorBuilder {
	constexpr explicit DoorBuilder(BlockType id) : m_id(id) {}

	DoorBuilder rotation(uint8_t rotation) const {
		DoorBuilder b = *this;
		b.m_rotation = rotation & 0x3;
		return b;
	}
	DoorBuilder open(bool open) const {
		DoorBuilder b = *this;
		b.m_open = open;
		return b;
	}
	DoorBuilder upperHalf(bool upperHalf) const {
		DoorBuilder b = *this;
		b.m_upperHalf = upperHalf;
		return b;
	}

	Block asBlock() const {
		uint8_t data = m_rotation & 0x3;
		if (m_open)
			data |= 0x4;
		if (m_upperHalf)
			data |= 0x8;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id;
	uint8_t m_rotation = 0;
	bool m_open = false;
	bool m_upperHalf = false;
};

struct SignBuilder {
	SignBuilder rotation(uint8_t rotation) const {
		SignBuilder b = *this;
		b.m_rotation = rotation & 0xF;
		return b;
	}

	Block asBlock() const {
		return Block{ m_id, static_cast<uint8_t>(m_rotation & 0xF) };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_SIGN;
	uint8_t m_rotation = 0;
};

struct RailBuilder {
	RailBuilder shape(RailShape shape) const {
		RailBuilder b = *this;
		b.m_shape = shape;
		return b;
	}

	Block asBlock() const {
		return Block{ m_id, static_cast<uint8_t>(m_shape) };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_RAIL;
	RailShape m_shape = RailShape::FlatNorthSouth;
};

struct LeverBuilder {
	LeverBuilder mount(LeverMount mount) const {
		LeverBuilder b = *this;
		b.m_mount = mount;
		return b;
	}
	LeverBuilder on(bool on) const {
		LeverBuilder b = *this;
		b.m_on = on;
		return b;
	}

	Block asBlock() const {
		uint8_t data = static_cast<uint8_t>(m_mount);
		if (m_on)
			data |= 0x8;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_LEVER;
	LeverMount m_mount = LeverMount::FloorEastWest;
	bool m_on = false;
};

struct ButtonBuilder {
	ButtonBuilder mount(ButtonMount mount) const {
		ButtonBuilder b = *this;
		b.m_mount = mount;
		return b;
	}
	ButtonBuilder pressed(bool pressed) const {
		ButtonBuilder b = *this;
		b.m_pressed = pressed;
		return b;
	}

	Block asBlock() const {
		uint8_t data = static_cast<uint8_t>(m_mount);
		if (m_pressed)
			data |= 0x8;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_BUTTON_STONE;
	ButtonMount m_mount = ButtonMount::WestWall;
	bool m_pressed = false;
};

struct SnowLayerBuilder {
	SnowLayerBuilder height(uint8_t height) const {
		SnowLayerBuilder b = *this;
		b.m_height = std::min<uint8_t>(height, 7);
		return b;
	}

	Block asBlock() const {
		return Block{ m_id, static_cast<uint8_t>(m_height & 0x7) };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_SNOW_LAYER;
	uint8_t m_height = 0;
};

struct TrapdoorBuilder {
	TrapdoorBuilder facing(Direction direction) const {
		TrapdoorBuilder b = *this;
		b.m_direction = direction;
		return b;
	}
	TrapdoorBuilder open(bool open) const {
		TrapdoorBuilder b = *this;
		b.m_open = open;
		return b;
	}

	Block asBlock() const {
		uint8_t data;
		switch (m_direction) {
		case Direction::South:
			data = 0;
			break;
		case Direction::North:
			data = 1;
			break;
		case Direction::East:
			data = 2;
			break;
		case Direction::West:
			data = 3;
			break;
		}
		if (m_open)
			data |= 0x4;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id = BLOCK_TRAPDOOR;
	Direction m_direction = Direction::South;
	bool m_open = false;
};

struct PumpkinBuilder {
	constexpr explicit PumpkinBuilder(BlockType id) : m_id(id) {}

	PumpkinBuilder facing(Direction direction) const {
		PumpkinBuilder b = *this;
		b.m_direction = direction;
		return b;
	}

	Block asBlock() const {
		uint8_t data;
		switch (m_direction) {
		case Direction::South:
			data = 0;
			break;
		case Direction::West:
			data = 1;
			break;
		case Direction::North:
			data = 2;
			break;
		case Direction::East:
			data = 3;
			break;
		}
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id;
	Direction m_direction = Direction::South;
};

struct RepeaterBuilder {
	constexpr explicit RepeaterBuilder(BlockType id) : m_id(id) {}

	RepeaterBuilder facing(Direction direction) const {
		RepeaterBuilder b = *this;
		b.m_direction = direction;
		return b;
	}
	RepeaterBuilder delay(uint8_t delay) const {
		RepeaterBuilder b = *this;
		b.m_delay = std::min<uint8_t>(delay, 3);
		return b;
	}

	int delayTicks() const {
		return (static_cast<int>(m_delay) + 1) * 2;
	}

	Block asBlock() const {
		uint8_t data;
		switch (m_direction) {
		case Direction::North:
			data = 0;
			break;
		case Direction::East:
			data = 1;
			break;
		case Direction::South:
			data = 2;
			break;
		case Direction::West:
			data = 3;
			break;
		}
		data |= (m_delay & 0x3) << 2;
		return Block{ m_id, data };
	}
	operator Block() const {
		return asBlock();
	}

private:
	BlockType m_id;
	Direction m_direction = Direction::North;
	uint8_t m_delay = 0;
};

inline constexpr FluidBuilder flowingWater{ BLOCK_WATER_FLOWING };
inline constexpr FluidBuilder water{ BLOCK_WATER_STILL };
inline constexpr FluidBuilder flowingLava{ BLOCK_LAVA_FLOWING };
inline constexpr FluidBuilder lava{ BLOCK_LAVA_STILL };
inline constexpr SaplingBuilder sapling{};
inline constexpr LogBuilder log{};
inline constexpr LeavesBuilder leaves{};
inline constexpr WallFacingBuilder dispenser{ BLOCK_DISPENSER };
inline constexpr BedBuilder bed{};
inline constexpr PoweredRailBuilder poweredRail{ BLOCK_RAIL_POWERED };
inline constexpr PoweredRailBuilder detectorRail{ BLOCK_RAIL_DETECTOR };
inline constexpr PistonBuilder stickyPiston{ BLOCK_PISTON_STICKY };
inline constexpr TallGrassBuilder tallGrass{};
inline constexpr PistonBuilder piston{ BLOCK_PISTON };
inline constexpr PistonHeadBuilder pistonHead{};
inline constexpr WoolBuilder wool{};
inline constexpr SlabBuilder doubleSlab{ BLOCK_DOUBLE_SLAB };
inline constexpr SlabBuilder slab{ BLOCK_SLAB };
inline constexpr TorchBuilder torch{ BLOCK_TORCH };
inline constexpr StairsBuilder woodenStairs{ BLOCK_STAIRS_WOOD };
inline constexpr WallFacingBuilder furnace{ BLOCK_FURNACE };
inline constexpr WallFacingBuilder litFurnace{ BLOCK_FURNACE_LIT };
inline constexpr SignBuilder sign{};
inline constexpr DoorBuilder woodenDoor{ BLOCK_DOOR_WOOD };
inline constexpr WallFacingBuilder ladder{ BLOCK_LADDER };
inline constexpr RailBuilder rail{};
inline constexpr StairsBuilder cobblestoneStairs{ BLOCK_STAIRS_COBBLESTONE };
inline constexpr WallFacingBuilder wallSign{ BLOCK_SIGN_WALL };
inline constexpr LeverBuilder lever{};
inline constexpr DoorBuilder ironDoor{ BLOCK_DOOR_IRON };
inline constexpr TorchBuilder redstoneTorch{ BLOCK_REDSTONE_TORCH_OFF };
inline constexpr TorchBuilder litRedstoneTorch{ BLOCK_REDSTONE_TORCH_ON };
inline constexpr ButtonBuilder stoneButton{};
inline constexpr SnowLayerBuilder snowLayer{};
inline constexpr PumpkinBuilder pumpkin{ BLOCK_PUMPKIN };
inline constexpr PumpkinBuilder jackOLantern{ BLOCK_PUMPKIN_LIT };
inline constexpr RepeaterBuilder redstoneRepeater{ BLOCK_REDSTONE_REPEATER_OFF };
inline constexpr RepeaterBuilder litRedstoneRepeater{ BLOCK_REDSTONE_REPEATER_ON };
inline constexpr TrapdoorBuilder trapdoor{};
} // namespace Blocks
