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
	TileType type;
	Int3 position{ 0, 0, 0 }; // Global coordinates
	bool canTick = false;
	Chunk* chunk =
	    nullptr; // The chunk this tile entity is in; may not be best practice to have this as a raw pointer but it should be fine since the chunk will always exist while the tile entity exists

	TileEntity(TileType pType, Int3 pPosition) : type(pType), position(pPosition) {};

	virtual void tick() {};
	virtual Tag serialize();
	virtual ~TileEntity() = default;
};

// Chest
struct TileEntityChest : TileEntity {
	InventoryChest inventory;
	TileEntityChest(Int3 pPosition) : TileEntity(TileType::CHEST, pPosition) {};

	Tag serialize() override;
	void tick() override;
};

// Furnace
struct TileEntityFurnace : TileEntity {
	InventoryFurnace inventory;
	TileEntityFurnace(Int3 pPosition) : TileEntity(TileType::FURNACE, pPosition) {
		canTick = true;
	};

	Tag serialize() override;
	void tick() override;
};

// Dispenser (Trap)
struct TileEntityDispenser : TileEntity {
	InventoryDispenser inventory;
	TileEntityDispenser(Int3 pPosition) : TileEntity(TileType::DISPENSER, pPosition) {};

	Tag serialize() override;
	void tick() override;
};

// Sign
struct TileEntitySign : TileEntity {
	std::string text1 = "";
	std::string text2 = "";
	std::string text3 = "";
	std::string text4 = "";
	TileEntitySign(Int3 pPosition) : TileEntity(TileType::SIGN, pPosition) {};

	Tag serialize() override;
};

// MobSpawner
struct TileEntityMobSpawner : TileEntity {
	std::string entityId = "";
	int16_t delay = 0;
	TileEntityMobSpawner(Int3 pPosition) : TileEntity(TileType::SPAWNER, pPosition) {
		canTick = true;
	};

	Tag serialize() override;
};