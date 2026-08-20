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
#include "packet_data.h"
#include <vector>

namespace PacketUtilities {
// Serialise once and copy the bytes to every session. Counts against each
// session's per-tick packet budget.
inline void BroadcastPacket(const Packet::BasePacket& _pkt, const std::vector<PlayerSession*>& _sessions) {
	if (_sessions.empty())
		return;
	if (_sessions.size() == 1) {
		_pkt.Serialize(_sessions[0]->stream);
		return;
	}
	thread_local NetworkStream scratch(-1);
	scratch.ClearWriteBuffer();
	_pkt.Serialize(scratch);
	auto pending = scratch.GetPendingWrite();
	for (auto* session : _sessions)
		session->stream.WriteRawPacket(pending.data(), pending.size(), _pkt.id);
}

inline void SendInventory(PlayerSession& _session, WindowId _windowId, Inventory& _inventory) {
	std::vector<ItemStack> items;
	items.reserve(_inventory.slots.size());
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