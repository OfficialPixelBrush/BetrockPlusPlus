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

// I hate doing inheritance but its simple to do for this
struct Chunk;
struct TileEntity {
	std::string m_id = "";
	Int3 m_position{ 0, 0, 0 }; // Global coordinates
	bool m_canTick = false;
	Chunk* m_chunk =
	    nullptr; // The chunk this tile entity is in; may not be best practice to have this as a raw pointer but it should be fine since the chunk will always exist while the tile entity exists

	TileEntity(std::string pId, Int3 pPosition) : m_id(pId), m_position(pPosition) {};

	virtual void tick() {};
	virtual Tag serialize();
	virtual ~TileEntity() = default;
};

// Chest
struct TileEntityChest : TileEntity {
	InventoryChest inventory;
	TileEntityChest(Int3 pPosition) : TileEntity("Chest", pPosition) {};

	Tag serialize() override;
	void tick() override;
};

// Furnace
struct TileEntityFurnace : TileEntity {
	InventoryFurnace inventory;
	TileEntityFurnace(Int3 pPosition) : TileEntity("Furnace", pPosition) {
		m_canTick = true;
	};

	Tag serialize() override;
	void tick() override;
};

// Dispenser (Trap)
struct TileEntityDispenser : TileEntity {
	InventoryDispenser inventory;
	TileEntityDispenser(Int3 pPosition) : TileEntity("Trap", pPosition) {};

	Tag serialize() override;
	void tick() override;
};

// Sign
struct TileEntitySign : TileEntity {
	std::string m_text1 = "";
	std::string m_text2 = "";
	std::string m_text3 = "";
	std::string m_text4 = "";
	TileEntitySign(Int3 pPosition) : TileEntity("Sign", pPosition) {};

	Tag serialize() override;
};

// MobSpawner
struct TileEntityMobSpawner : TileEntity {
	std::string m_entityId = "";
	int16_t m_delay = 0;
	TileEntityMobSpawner(Int3 pPosition) : TileEntity("MobSpawner", pPosition) {
		m_canTick = true;
	};

	Tag serialize() override;
};