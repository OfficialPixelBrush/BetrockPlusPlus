/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "furnace.h"
#include "tile_entities/tile_entity.h"

// Furnace tile-entity slot indices: 0 = input, 1 = fuel, 2 = output
static constexpr int RESULT_SLOT = 2;

FurnaceInventoryInteraction::FurnaceInventoryInteraction(InventoryPlayer* _pinv,
                                                         std::shared_ptr<TileEntityFurnace> _tile)
    : InventoryInteraction(&sharedInventory), playerInventory(_pinv), tile(_tile), furnaceInventory(&_tile->inventory) {
	sharedInventory.owner = this;
	MergeInventories();
}

FurnaceInventoryInteraction::~FurnaceInventoryInteraction() {
	if (!tile.expired())
		WriteBack();
}

bool FurnaceInventoryInteraction::CanExist(PlayerEntity& player) {
	if (tile.expired())
		return false;
	auto pos = tile.lock()->position;
	return (GetDistSquared(player.position, {pos.x + 0.5, pos.y + 0.5, pos.z + 0.5}) < 64.0);
}

void FurnaceInventoryInteraction::InitSnapshot() {
	snapshot.resize(size_t(furnaceInventory->GetSizeInventory()));
	for (size_t i = 0; i < size_t(furnaceInventory->GetSizeInventory()); i++)
		snapshot[i] = furnaceInventory->slots[i];
}

// Analyze the snapshot vs the current furnace inventory
std::vector<DeltaSlot> FurnaceInventoryInteraction::TickDiff() {
	std::vector<DeltaSlot> differences;
	for (size_t i = 0; i < snapshot.size(); i++) {
		auto& snap = snapshot[i];

		bool changed = snap != furnaceInventory->slots[i];
		if (!changed)
			continue;

		snap = furnaceInventory->slots[i];
		differences.push_back({ snap, int(i) });
	}
	MergeInventories();
	return differences;
}

void FurnaceInventoryInteraction::MergeInventories() {
	size_t slotCount = 0;
	for (auto& slot : furnaceInventory->slots)
		sharedInventory.slots[slotCount++] = slot;
	for (size_t i = 9; i < 45; i++)
		sharedInventory.slots[slotCount++] = playerInventory->slots[i];
}

void FurnaceInventoryInteraction::WriteBack() {
	for (size_t i = 0; i < 3; i++)
		furnaceInventory->slots[i] = sharedInventory.slots[i];
	// shared index i (>=3) maps back to playerInventory index i + 6,
	// since MergeInventories() reads playerInventory[9..44] starting at shared index 3.
	for (size_t i = 3; i < 39; i++)
		playerInventory->slots[i + 6] = sharedInventory.slots[i];
}

void FurnaceInventoryInteraction::TakeResult() {
	ItemStack& result = furnaceInventory->slots[RESULT_SLOT];
	if (result.id == Items::Id::INVALID)
		return;

	if (carried.id == Items::Id::INVALID) {
		carried = result;
	} else if (carried.id == result.id && carried.data == result.data) {
		// Same type, try merge with cursor
		int maxStack = Items::GetMaxStack(carried.id);
		if (int(carried.count) + int(result.count) > maxStack)
			return;
		carried.count += result.count;
	} else {
		// Cursor holds something else
		return;
	}

	// Clear the result slot
	furnaceInventory->slots[RESULT_SLOT] = ItemStack{};
	MergeInventories();
}

void FurnaceInventoryInteraction::OnLeftClick(int _slot) {
	if (_slot == RESULT_SLOT) {
		TakeResult();
		return;
	}
	InventoryInteraction::OnLeftClick(_slot);
}

void FurnaceInventoryInteraction::OnRightClick(int _slot) {
	if (_slot == RESULT_SLOT) {
		TakeResult();
		return;
	}
	InventoryInteraction::OnRightClick(_slot);
}

void FurnaceInventoryInteraction::OnShiftClick(int _slot) {
	if (_slot == RESULT_SLOT) {
		ShiftClickResult();
		return;
	}

	auto stack = sharedInventory.GetStackInSlot(_slot);
	if (!stack)
		return;

	ItemStack copy = *stack;

	if (_slot < 3) {
		// Furnace (input/fuel) -> inventory
		// Try the main inventory then the hotbar
		bool success = playerInventory->MergeItemStackInInventory(copy, false, 9, 35);
		if (!success)
			playerInventory->MergeItemStackInInventory(copy, false, 36, 44);
	} else {
		// We can't shift click into the furnace slots themselves, so just try the other area of the inventory
		// We shift clicked in the inventory
		if (_slot >= 3 && _slot < 30) {
			// shared [3,30) == playerInventory main inventory [9,35]
			playerInventory->MergeItemStackInInventory(copy, false, 36, 44);
		} else {
			// shared [30,39) == playerInventory hotbar [36,44]
			playerInventory->MergeItemStackInInventory(copy, false, 9, 35);
		}
	}

	// Update the source in the real inventory before re-merging
	if (_slot < 3) {
		furnaceInventory->slots[size_t(_slot)] = copy.count == 0 ? ItemStack{} : copy;
	} else {
		playerInventory->slots[size_t(_slot) + 6] = copy.count == 0 ? ItemStack{} : copy;
	}

	// Re-sync sharedInventory from the real inventories
	MergeInventories();
}

void FurnaceInventoryInteraction::ShiftClickResult() {
	ItemStack& result = furnaceInventory->slots[RESULT_SLOT];
	if (result.id == Items::Id::INVALID)
		return;

	ItemStack copy = result;
	if (playerInventory->MergeItemStackInInventory(copy, true, 9, 44)) {
		// Successfully moved result to inventory
		furnaceInventory->slots[RESULT_SLOT] = ItemStack{};
	} else {
		// Couldn't move all, update with remaining
		furnaceInventory->slots[RESULT_SLOT] = copy.count == 0 ? ItemStack{} : copy;
	}
	MergeInventories();
}