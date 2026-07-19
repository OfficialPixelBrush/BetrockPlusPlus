/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include <cstdint>

// Map colors
struct MapColor {
	uint8_t m_index = 0;
	uint32_t m_colorValue = 0; // packed RGB

	static constexpr MapColor Air() {
		return { 0, 0x000000 };
	}
	static constexpr MapColor Grass() {
		return { 1, 0x7FB238 };
	}
	static constexpr MapColor Sand() {
		return { 2, 0xF7E9A3 };
	}
	static constexpr MapColor Cloth() {
		return { 3, 0xA7A7A7 };
	}
	static constexpr MapColor TNT() {
		return { 4, 0xFF0000 };
	}
	static constexpr MapColor Ice() {
		return { 5, 0xA0A0FF };
	}
	static constexpr MapColor Iron() {
		return { 6, 0xA7A7A7 };
	}
	static constexpr MapColor Foliage() {
		return { 7, 0x007C00 };
	}
	static constexpr MapColor Snow() {
		return { 8, 0xFFFFFF };
	}
	static constexpr MapColor Clay() {
		return { 9, 0xA4A8B8 };
	}
	static constexpr MapColor Dirt() {
		return { 10, 0xB7906F };
	}
	static constexpr MapColor Stone() {
		return { 11, 0x707070 };
	}
	static constexpr MapColor Water() {
		return { 12, 0x4040FF };
	}
	static constexpr MapColor Wood() {
		return { 13, 0x685432 };
	}
};

enum class MaterialType : uint8_t {
	Air,
	Grass,
	Ground, // dirt, gravel, clay
	Wood,
	Rock,
	Iron,
	Water,
	Lava,
	Leaves,
	Plants, // non-solid, no grass cover (flowers, saplings, etc)
	Sponge,
	Cloth, // wool, web
	Fire,
	Sand,
	Circuits, // redstone wire, rails, etc
	Glass,
	TNT,
	Coral,
	Ice,
	SnowLayer, // snow layer
	SnowBlock, // snow block
	Cactus,
	Clay,
	Pumpkin,
	Portal,
	Cake,
	Web,
	Piston,
};

enum PushabilityFlag : uint8_t {
	Normal = 0,
	NoPush = 1,
	Immovable = 2,
};

struct Material {
	MaterialType m_type = MaterialType::Rock;
	MapColor m_mapColor = MapColor::Stone();
	bool m_isLiquid = false;
	bool m_isSolid = true;
	bool m_isOpaque = true;
	bool m_canBurn = false;
	bool m_isGroundCover = false;
	bool m_canBlockGrass = true;
	bool m_isHarvestable = true;
	PushabilityFlag m_mobilityFlag = PushabilityFlag::Normal;

	bool operator==(const Material& other) const {
		return m_type == other.m_type;
	}
	bool operator!=(const Material& other) const {
		return m_type != other.m_type;
	}

	static constexpr Material Air() {
		Material m{};
		m.m_type = MaterialType::Air;
		m.m_mapColor = MapColor::Air();
		m.m_isSolid = false;
		m.m_isOpaque = false;
		m.m_canBlockGrass = false;
		m.m_isGroundCover = true;
		return m;
	}

	static constexpr Material Grass() {
		Material m{};
		m.m_type = MaterialType::Grass;
		m.m_mapColor = MapColor::Grass();
		return m;
	}

	static constexpr Material Ground() {
		Material m{};
		m.m_type = MaterialType::Ground;
		m.m_mapColor = MapColor::Dirt();
		return m;
	}

	static constexpr Material Wood() {
		Material m{};
		m.m_type = MaterialType::Wood;
		m.m_mapColor = MapColor::Wood();
		m.m_canBurn = true;
		return m;
	}

	static constexpr Material Rock() {
		Material m{};
		m.m_type = MaterialType::Rock;
		m.m_mapColor = MapColor::Stone();
		m.m_isHarvestable = false;
		return m;
	}

	static constexpr Material Iron() {
		Material m{};
		m.m_type = MaterialType::Iron;
		m.m_mapColor = MapColor::Iron();
		m.m_isHarvestable = false;
		return m;
	}

	static constexpr Material Water() {
		Material m{};
		m.m_type = MaterialType::Water;
		m.m_mapColor = MapColor::Water();
		m.m_isLiquid = true;
		m.m_isSolid = false;
		m.m_isGroundCover = true;
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material Lava() {
		Material m{};
		m.m_type = MaterialType::Lava;
		m.m_mapColor = MapColor::TNT();
		m.m_isLiquid = true;
		m.m_isSolid = false;
		m.m_isGroundCover = true;
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material Leaves() {
		Material m{};
		m.m_type = MaterialType::Leaves;
		m.m_mapColor = MapColor::Foliage();
		m.m_canBurn = true;
		m.m_isOpaque = false;
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material Plants() {
		Material m{};
		m.m_type = MaterialType::Plants;
		m.m_mapColor = MapColor::Foliage();
		m.m_isSolid = false;
		m.m_isOpaque = false;
		m.m_canBlockGrass = false;
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material Sponge() {
		Material m{};
		m.m_type = MaterialType::Sponge;
		m.m_mapColor = MapColor::Cloth();
		return m;
	}

	static constexpr Material Cloth() {
		Material m{};
		m.m_type = MaterialType::Cloth;
		m.m_mapColor = MapColor::Cloth();
		m.m_canBurn = true;
		return m;
	}

	static constexpr Material Fire() {
		Material m{};
		m.m_type = MaterialType::Fire;
		m.m_mapColor = MapColor::Air();
		m.m_isSolid = false;
		m.m_isOpaque = false;
		m.m_canBlockGrass = false;
		m.m_isGroundCover = true;
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material Sand() {
		Material m{};
		m.m_type = MaterialType::Sand;
		m.m_mapColor = MapColor::Sand();
		return m;
	}

	// MaterialLogic + setNoPushMobility
	static constexpr Material Circuits() {
		Material m{};
		m.m_type = MaterialType::Circuits;
		m.m_mapColor = MapColor::Air();
		m.m_isSolid = false;
		m.m_isOpaque = false;
		m.m_canBlockGrass = false;
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material Glass() {
		Material m{};
		m.m_type = MaterialType::Glass;
		m.m_mapColor = MapColor::Air();
		m.m_isOpaque = false;
		return m;
	}

	static constexpr Material TNT() {
		Material m{};
		m.m_type = MaterialType::TNT;
		m.m_mapColor = MapColor::TNT();
		m.m_canBurn = true;
		m.m_isOpaque = false;
		return m;
	}

	static constexpr Material Coral() {
		Material m{};
		m.m_type = MaterialType::Coral;
		m.m_mapColor = MapColor::Foliage();
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material Ice() {
		Material m{};
		m.m_type = MaterialType::Ice;
		m.m_mapColor = MapColor::Ice();
		m.m_isOpaque = false;
		return m;
	}

	static constexpr Material SnowLayer() {
		Material m{};
		m.m_type = MaterialType::SnowLayer;
		m.m_mapColor = MapColor::Snow();
		m.m_isSolid = false;
		m.m_isOpaque = false;
		m.m_isGroundCover = true;
		m.m_isHarvestable = false;
		m.m_canBlockGrass = false;
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material SnowBlock() {
		Material m{};
		m.m_type = MaterialType::SnowBlock;
		m.m_mapColor = MapColor::Snow();
		m.m_isHarvestable = false;
		return m;
	}

	static constexpr Material Cactus() {
		Material m{};
		m.m_type = MaterialType::Cactus;
		m.m_mapColor = MapColor::Foliage();
		m.m_isOpaque = false;
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material Clay() {
		Material m{};
		m.m_type = MaterialType::Clay;
		m.m_mapColor = MapColor::Clay();
		return m;
	}

	static constexpr Material Pumpkin() {
		Material m{};
		m.m_type = MaterialType::Pumpkin;
		m.m_mapColor = MapColor::Foliage();
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material Portal() {
		Material m{};
		m.m_type = MaterialType::Portal;
		m.m_mapColor = MapColor::Air();
		m.m_isSolid = false;
		m.m_isOpaque = false;
		m.m_canBlockGrass = false;
		m.m_mobilityFlag = PushabilityFlag::Immovable;
		return m;
	}

	static constexpr Material Cake() {
		Material m{};
		m.m_type = MaterialType::Cake;
		m.m_mapColor = MapColor::Air();
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material Web() {
		Material m{};
		m.m_type = MaterialType::Web;
		m.m_mapColor = MapColor::Cloth();
		m.m_isHarvestable = false;
		m.m_mobilityFlag = PushabilityFlag::NoPush;
		return m;
	}

	static constexpr Material Piston() {
		Material m{};
		m.m_type = MaterialType::Piston;
		m.m_mapColor = MapColor::Stone();
		m.m_isHarvestable = false;
		m.m_mobilityFlag = PushabilityFlag::Immovable;
		return m;
	}
};