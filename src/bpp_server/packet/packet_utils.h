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
	for (auto& item : inventory.m_slots) {
		items.emplace_back(item.m_id, item.m_count, item.m_data);
	}
	Packet::FillContainer fc;
	fc.m_window_id = windowId;
	fc.m_items = std::move(items);
	fc.Serialize(session.m_stream);
}

// Sends a single slot update. windowId=-1 / slotId=-1 updates the cursor.
inline void sendSlot(PlayerSession& session, WindowId windowId, NetworkSlotId slotId, ItemStack* stack) {
	Packet::SetSlot pkt;
	pkt.m_window_id = windowId;
	pkt.m_slot_id = slotId;
	pkt.m_item = stack ? ItemStack{ stack->m_id, stack->m_count, stack->m_data } : ItemStack{ Items::Id::INVALID };
	pkt.Serialize(session.m_stream);
}

inline void CloseContainer(PlayerSession& session) {
	// Get rid of our active interaction and reset the window id
	Packet::CloseContainer cc;
	cc.m_window_id = session.m_openWindowId;
	cc.Serialize(session.m_stream);
	session.m_activeInteraction = nullptr;
	session.m_openWindowId = 0;
}
}; // namespace PacketUtilities