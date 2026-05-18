/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "numeric_structs.h"
#include "inventory/inventories.h"

// I hate doing inheritance but its simple to do for this
struct TileEntity {
	std::string id = "";
	Int3 position{ 0, 0, 0 }; // Global coordinates
	bool canTick = false;

	TileEntity(std::string pId, Int3 pPosition) : id(pId), position(pPosition) {};

	virtual void tick() {};
	virtual ~TileEntity() = default;
};

// Chest
struct TileEntityChest : TileEntity {
	InventoryChest inventory;
	TileEntityChest(Int3 pPosition) : TileEntity("Chest", pPosition) {};
};