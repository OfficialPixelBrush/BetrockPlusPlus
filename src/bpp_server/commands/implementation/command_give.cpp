/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../../packet/handle_packet.h"
#include "../command.h"
#include "../command_manager.h"
#include "inventory/item_stack.h"
#include "items.h"
#include "strings/labels.h"
#include <string>

// Give yourself a block or item
// Usage:
//   /give <id>[:meta] [amount]
std::string CommandGive::Execute(std::vector<std::string>& parameters, PlayerSession& session, WorldManager& world,
                                 std::function<void(PlayerSession&)> transferDimension) {
	// TODO: Let player specify another player to give to
	if (parameters.size() <= 1)

		return "Missing item id!";

	ItemStack item;
	const std::string& itemArg = parameters[1];
	size_t colonPos = itemArg.find(':');
	std::string idString = itemArg.substr(0, colonPos);
	std::string metaString = "";
	if (colonPos != std::string::npos) {
		metaString = itemArg.substr(colonPos + 1);
	}
	item.id = static_cast<int16_t>(std::stoi(idString));
	if (!metaString.empty()) {
		item.data = static_cast<int8_t>(std::stoi(metaString));
	}
	item.count = GetMaxStack(item.id); // I don't want 64 pickaxes anymore!!
	if (parameters.size() > 2) {
		item.count = static_cast<int8_t>(std::stoi(parameters[2]));
	}

	// Check if its even a valid item
	if ((item.id > BLOCK_AIR && item.id < BLOCK_MAX) || (item.id >= ITEM_SHOVEL_IRON && item.id < ITEM_MAX) ||
	    (item.id >= ITEM_RECORD_13 && item.id < ITEM_RECORD_MAX)) {
		Packet::ChatMessage reply;
		reply.message = "§eGave " + wIdToLabel(item.id) + " (" + std::to_string(item.id) + ":" +
		                std::to_string(item.data) + ") x" + std::to_string(item.count) + " to " + session.username;

		reply.Serialize(session.stream);

		// Try the hotbar
		if (session.inventory.mergeItemStackInInventory(item, false, 36, 44)) {
			PacketUtilities::sendInventory(session, session.openWindowId, session.inventory);
			return "";
		}

		// Try the main inventory
		if (session.inventory.mergeItemStackInInventory(item, false, 9, 35)) {
			PacketUtilities::sendInventory(session, session.openWindowId, session.inventory);
			return "";
		}

		// TODO: Drop on the ground
		return "";
	}
	return std::to_string(item.id) + " is not a valid item id!";
}