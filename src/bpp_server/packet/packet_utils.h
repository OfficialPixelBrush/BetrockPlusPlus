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
inline void sendInventory(PlayerSession& session, WindowId windowId, Inventory inventory) {
	std::vector<ItemStack> items;
	for (auto& item : inventory.slots) {
		items.emplace_back(item.id, item.count, item.data);
	}
	Packet::FillContainer fc;
	fc.window_id = windowId;
	fc.items = std::move(items);
	fc.Serialize(session.stream);
}

// Sends a single slot update. windowId=-1 / slotId=-1 updates the cursor.
inline void sendSlot(PlayerSession& session, WindowId windowId, NetworkSlotId slotId, ItemStack* stack) {
	Packet::SetSlot pkt;
	pkt.window_id = windowId;
	pkt.slot_id = slotId;
	pkt.item = stack ? ItemStack{ stack->id, stack->count, stack->data } : ItemStack{ Items::Id::INVALID };
	pkt.Serialize(session.stream);
}

inline void CloseContainer(PlayerSession& session) {
	// Get rid of our active interaction and reset the window id
	Packet::CloseContainer cc;
	cc.window_id = session.openWindowId;
	cc.Serialize(session.stream);
	session.activeInteraction = nullptr;
	session.openWindowId = 0;
}
}; // namespace PacketUtilities