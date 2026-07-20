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

	TileEntity(TileType _pType, Int3 _pPosition) : type(_pType), position(_pPosition) {};

	virtual void Tick() {};
	virtual Tag Serialize();
	virtual ~TileEntity() = default;
};

// Chest
struct TileEntityChest : TileEntity {
	InventoryChest inventory;
	TileEntityChest(Int3 _pPosition) : TileEntity(TileType::CHEST, _pPosition) {};

	Tag Serialize() override;
	void Tick() override;
};

// Furnace
struct TileEntityFurnace : TileEntity {
	InventoryFurnace inventory;
	TileEntityFurnace(Int3 _pPosition) : TileEntity(TileType::FURNACE, _pPosition) {
		canTick = true;
	};

	Tag Serialize() override;
	void Tick() override;
};

// Dispenser (Trap)
struct TileEntityDispenser : TileEntity {
	InventoryDispenser inventory;
	TileEntityDispenser(Int3 _pPosition) : TileEntity(TileType::DISPENSER, _pPosition) {};

	Tag Serialize() override;
	void Tick() override;
};

// Sign
struct TileEntitySign : TileEntity {
	std::string text1 = "";
	std::string text2 = "";
	std::string text3 = "";
	std::string text4 = "";
	TileEntitySign(Int3 _pPosition) : TileEntity(TileType::SIGN, _pPosition) {};

	Tag Serialize() override;
};

// MobSpawner
struct TileEntityMobSpawner : TileEntity {
	std::string entityId = "";
	int16_t delay = 0;
	TileEntityMobSpawner(Int3 _pPosition) : TileEntity(TileType::SPAWNER, _pPosition) {
		canTick = true;
	};

	Tag Serialize() override;
};