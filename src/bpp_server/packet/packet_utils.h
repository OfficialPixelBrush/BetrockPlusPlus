/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "../player_conn/player_session.h"
#include "networking/packets.h"
#include <vector>

namespace PacketUtilities {
inline void SendInventory(PlayerSession& _session, WindowId _windowId, Inventory _inventory) {
	std::vector<ItemStack> items;
	for (auto& item : _inventory.slots) {
		items.emplace_back(item.id, item.count, item.data);
	}
	Packet::FillContainer fc;
	fc.windowId = _windowId;
	fc.items = std::move(items);
	fc.Serialize(_session.stream);
}

// Sends a single slot update. windowId=-1 / slotId=-1 updates the cursor.
inline void SendSlot(PlayerSession& _session, WindowId _windowId, NetworkSlotId _slotId, ItemStack* _stack) {
	Packet::SetSlot pkt;
	pkt.windowId = _windowId;
	pkt.slotId = _slotId;
	pkt.item = _stack ? ItemStack{ _stack->id, _stack->count, _stack->data } : ItemStack{ Items::Id::INVALID };
	pkt.Serialize(_session.stream);
}

inline void CloseContainer(PlayerSession& _session) {
	// Get rid of our active interaction and reset the window id
	Packet::CloseContainer cc;
	cc.windowId = _session.openWindowId;
	cc.Serialize(_session.stream);
	_session.activeInteraction = nullptr;
	_session.openWindowId = 0;
}
}; // namespace PacketUtilities