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
#include <string>

// Give yourself a block or item
// Usage:
//   /give <id>[:meta] [amount]
std::string CommandGive::Execute(std::vector<std::string>& parameters, PlayerSession& session,
                                 [[maybe_unused]] WorldManager& world,
                                 [[maybe_unused]] std::function<void(PlayerSession&)> transferDimension,
                                 [[maybe_unused]] Server& server) {
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
	item.m_id = static_cast<int16_t>(std::stoi(idString));
	if (!metaString.empty()) {
		item.m_data = static_cast<int16_t>(std::stoi(metaString));
	}
	item.m_count = Items::GetMaxStack(item.m_id); // I don't want 64 pickaxes anymore!!
	if (parameters.size() > 2) {
		item.m_count = static_cast<int8_t>(std::stoi(parameters[2]));
	}

	// Check if its even a valid item
	if ((item.m_id > BLOCK_AIR && item.m_id < BLOCK_MAX) || Items::IsValid(item.m_id)) {
		Packet::ChatMessage reply;
		reply.m_message = "§eGave " + wIdToLabel(item.m_id) + " (" + std::to_string(item.m_id) + ":" +
		                std::to_string(item.m_data) + ") x" + std::to_string(item.m_count) + " to " + session.m_username;

		reply.Serialize(session.m_stream);

		// Try the hotbar
		if (session.m_inventory.mergeItemStackInInventory(item, false, 36, 44)) {
			PacketUtilities::sendInventory(session, session.m_openWindowId, session.m_inventory);
			return "";
		}

		// Try the main inventory
		if (session.m_inventory.mergeItemStackInInventory(item, false, 9, 35)) {
			PacketUtilities::sendInventory(session, session.m_openWindowId, session.m_inventory);
			return "";
		}

		// TODO: Drop on the ground
		return "";
	}
	return std::to_string(item.m_id) + " is not a valid item id!";
}