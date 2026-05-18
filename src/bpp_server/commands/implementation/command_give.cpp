/*

 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>

 *

 * SPDX-License-Identifier: GPL-3.0-only

*/



#include "../command.h"

#include "../command_manager.h"

#include "inventory/item_stack.h"

#include "../../handle_packet.h"

#include "items.h"

#include "strings/labels.h"

#include <string>



// Give yourself a block or item

// Usage:

//   /give <id>[:meta] [amount]

std::wstring CommandGive::Execute(std::vector<std::wstring>& parameters, PlayerSession& session, WorldManager& world, std::function<void(PlayerSession&)> transferDimension)

{

	// TODO: Let player specify another player to give to

    if (parameters.size() <= 1)

        return L"Missing item id!";



    ItemStack item;



    const std::wstring& itemArg = parameters[1];



    size_t colonPos = itemArg.find(L':');



    std::wstring idString = itemArg.substr(0, colonPos);



    std::wstring metaString = L"";



    if (colonPos != std::wstring::npos) {

        metaString = itemArg.substr(colonPos + 1);

    }



    item.id = static_cast<int16_t>(std::stoi(idString));



    if (!metaString.empty()) {

        item.data = static_cast<int8_t>(std::stoi(metaString));

    }



    item.count = ITEM_STACK_MAX;



    if (parameters.size() > 2) {

        item.count = static_cast<int8_t>(std::stoi(parameters[2]));

    }



	// Check if its even a valid item

    if (

		(item.id > BLOCK_AIR && item.id < BLOCK_MAX) ||

        (item.id >= ITEM_SHOVEL_IRON && item.id < ITEM_MAX) ||

        (item.id >= ITEM_RECORD_13 && item.id < ITEM_RECORD_MAX)

	)

    {

		Packet::ChatMessage reply;

		reply.message =

			L"\u00a77Gave " +

			wIdToLabel(item.id) +

			L" (" +

			std::to_wstring(item.id) +

			L":" +

			std::to_wstring(item.data) +

			L") x" +

			std::to_wstring(item.count) +

			L" to " +

			session.username

		;

		reply.Serialize(session.stream);

		// Try the hotbar

        if (session.inventory.mergeItemStackInInventory(item, false, 36, 44)) {

			PacketUtilities::sendInventory(session, session.openWindowId, session.inventory);

			return L"";

		}

		// Try the main inventory

		if (session.inventory.mergeItemStackInInventory(item, false, 9, 35)) {

			PacketUtilities::sendInventory(session, session.openWindowId, session.inventory);

			return L"";

		}

		// TODO: Drop on the ground

        return L"";

    }



    return std::to_wstring(item.id) + L" is not a valid item id!";

}