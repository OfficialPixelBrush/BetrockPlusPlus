/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "inventory_interaction.h"

InventoryInteraction::InventoryInteraction(Inventory* inv) : m_inventory(inv) {}

bool InventoryInteraction::canExist() {
	return m_inventory != nullptr;
}

void InventoryInteraction::initSnapshot() {
	m_snapshot = m_inventory->m_slots;
}

std::vector<DeltaSlot> InventoryInteraction::tickDiff() {
	std::vector<DeltaSlot> differences;
	for (size_t i = 0; i < m_snapshot.size(); i++) {
		[[maybe_unused]] auto* current = m_inventory->getStackInSlot(i);
		auto& snap = m_snapshot[i];

		bool changed = snap != m_inventory->m_slots[i];
		if (!changed)
			continue;

		snap = m_inventory->m_slots[i];
		differences.push_back({ snap, int(i) });
	}
	return differences;
}

void InventoryInteraction::onLeftClick(int slot) {
	auto targetSlot = m_inventory->getStackInSlot(slot);

	// Empty slot
	if (!targetSlot) {
		if (m_carried.m_id != Items::Id::INVALID) {
			m_inventory->setInventorySlotContents(slot, &m_carried);
			m_carried = ItemStack{};
		}
		m_inventory->onInventoryChanged();
		return;
	}

	// Not carrying anything
	if (m_carried.m_id == Items::Id::INVALID) {
		m_carried = *targetSlot;
		m_inventory->clearSlot(slot);
		m_inventory->onInventoryChanged();
		return;
	}

	// Same item; merge
	if (targetSlot->m_id == m_carried.m_id && targetSlot->m_data == m_carried.m_data) {
		int maxStack = Items::GetMaxStack(targetSlot->m_id);
		int space = maxStack - targetSlot->m_count;
		int toMove = std::min(space, (int)m_carried.m_count);
		targetSlot->m_count += toMove;
		m_carried.m_count -= toMove;
		if (m_carried.m_count == 0)
			m_carried = ItemStack{};
		m_inventory->onInventoryChanged();
		return;
	}

	// Different item; swap
	ItemStack temp = *targetSlot;
	*targetSlot = m_carried;
	m_carried = temp;
	m_inventory->onInventoryChanged();
}

void InventoryInteraction::onRightClick(int slot) {
	auto targetSlot = m_inventory->getStackInSlot(slot);

	if (m_carried.m_id != Items::Id::INVALID) {
		if (!targetSlot) {
			ItemStack single{ m_carried.m_id, 1, m_carried.m_data };
			m_inventory->setInventorySlotContents(slot, &single);
			m_carried.m_count -= 1;
			if (m_carried.m_count == 0)
				m_carried = ItemStack{};
			m_inventory->onInventoryChanged();
			return;
		}

		// If we right click on the same item we are carrying just add one
		if (targetSlot->m_id == m_carried.m_id && targetSlot->m_data == m_carried.m_data) {
			int maxStack = Items::GetMaxStack(targetSlot->m_id);
			int space = maxStack - targetSlot->m_count;
			if (space >= 1) {
				targetSlot->m_count += 1;
				m_carried.m_count -= 1;
				if (m_carried.m_count == 0)
					m_carried = ItemStack{};
				m_inventory->onInventoryChanged();
			}
			return;
		}

		// If we right click on a different item, swap the cursor and that item
		ItemStack temp = *targetSlot;
		*targetSlot = m_carried;
		m_carried = temp;
		m_inventory->onInventoryChanged();
		return;
	}

	if (!targetSlot)
		return;

	// Only split items if there stack count is greater than 1 and we aren't carrying anything
	if (targetSlot->m_count > 1) {
		// Beta always take the higher of the two if uneven
		int taken = (targetSlot->m_count + 1) / 2;
		int left = targetSlot->m_count - taken;
		targetSlot->m_count = int8_t(left);
		m_carried = ItemStack{ targetSlot->m_id, int8_t(taken), targetSlot->m_data };
		m_inventory->onInventoryChanged();
		return;
	}

	// If its only one item we just pick it up
	m_carried = *targetSlot;
	m_inventory->clearSlot(slot);
	m_inventory->onInventoryChanged();
	return;
}

void InventoryInteraction::onShiftClick(int slot) {
	auto targetSlot = m_inventory->getStackInSlot(slot);
	if (!targetSlot)
		return;
	m_inventory->mergeItemStackInInventory(*targetSlot);
}
