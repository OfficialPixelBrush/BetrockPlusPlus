/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "inventory/inventories.h"
#include "nbt/nbt.h"
#include "numeric_structs.h"

enum class TileType {
	CHEST,
	FURNACE,
	DISPENSER,
	SIGN,
	SPAWNER
};

// I hate doing inheritance but its simple to do for this
struct Chunk;
struct TileEntity {
	TileType m_type;
	Int3 m_position{ 0, 0, 0 }; // Global coordinates
	bool m_canTick = false;
	Chunk* m_chunk =
	    nullptr; // The chunk this tile entity is in; may not be best practice to have this as a raw pointer but it should be fine since the chunk will always exist while the tile entity exists

	TileEntity(TileType pType, Int3 pPosition) : m_type(pType), m_position(pPosition) {};

	virtual void tick() {};
	virtual Tag serialize();
	virtual ~TileEntity() = default;
};

// Chest
struct TileEntityChest : TileEntity {
	InventoryChest m_inventory;
	TileEntityChest(Int3 pPosition) : TileEntity(TileType::CHEST, pPosition) {};

	Tag serialize() override;
	void tick() override;
};

// Furnace
struct TileEntityFurnace : TileEntity {
	InventoryFurnace m_inventory;
	TileEntityFurnace(Int3 pPosition) : TileEntity(TileType::FURNACE, pPosition) {
		m_canTick = true;
	};

	Tag serialize() override;
	void tick() override;
};

// Dispenser (Trap)
struct TileEntityDispenser : TileEntity {
	InventoryDispenser m_inventory;
	TileEntityDispenser(Int3 pPosition) : TileEntity(TileType::DISPENSER, pPosition) {};

	Tag serialize() override;
	void tick() override;
};

// Sign
struct TileEntitySign : TileEntity {
	std::string m_text1 = "";
	std::string m_text2 = "";
	std::string m_text3 = "";
	std::string m_text4 = "";
	TileEntitySign(Int3 pPosition) : TileEntity(TileType::SIGN, pPosition) {};

	Tag serialize() override;
};

// MobSpawner
struct TileEntityMobSpawner : TileEntity {
	std::string m_entityId = "";
	int16_t m_delay = 0;
	TileEntityMobSpawner(Int3 pPosition) : TileEntity(TileType::SPAWNER, pPosition) {
		m_canTick = true;
	};

	Tag serialize() override;
};