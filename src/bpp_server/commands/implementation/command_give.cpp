/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../../packet/packet_utils.h"
#include "../command.h"
#include "../command_manager.h"
#include "inventory/item_stack.h"
#include "items.h"
#include "strings/labels.h"
#include <cstddef>
#include <string>

// Give yourself a block or item
// Usage:
//   /give <id>[:meta] [amount]
std::string CommandGive::Execute(std::vector<std::string>& _parameters, PlayerSession& _session,
                                 [[maybe_unused]] WorldManager& _world,
                                 [[maybe_unused]] std::function<void(PlayerSession&)> _transferDimension,
                                 [[maybe_unused]] Server& _server) {
	// TODO: Let player specify another player to give to
	if (_parameters.size() <= 1)

		return "Missing item id!";
	
	size_t paramOffset = 1;
	ItemStack item = ParseItemStack(_parameters, paramOffset, true);

	// Check if its even a valid item
	if (Items::IsValidId(item.id)) {
		Packet::ChatMessage reply;
		reply.message = "§eGave " + WIdToLabel(item.id) + " (" + std::to_string(item.id) + ":" +
		                std::to_string(item.data) + ") x" + std::to_string(item.count) + " to " + _session.username;

		reply.Serialize(_session.stream);

		// Try the hotbar
		if (_session.inventory.MergeItemStackInInventory(item, false, 36, 44)) {
			PacketUtilities::SendInventory(_session, _session.openWindowId, _session.inventory);
			return "";
		}

		// Try the main inventory
		if (_session.inventory.MergeItemStackInInventory(item, false, 9, 35)) {
			PacketUtilities::SendInventory(_session, _session.openWindowId, _session.inventory);
			return "";
		}

		// TODO: Drop on the ground
		return "";
	}
	return std::to_string(item.id) + " is not a valid item id!";
}