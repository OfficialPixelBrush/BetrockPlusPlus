/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../../packet/packet_utils.h"
#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "items.h"
#include "strings/labels.h"

namespace {

std::string GiveItem(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto itemArg = _cmd.get_arg<std::string>("item");
	if (!itemArg)
		return "Missing item id!";

	std::optional<int> count;
	if (auto amount = _cmd.get_arg<int>("amount"))
		count = *amount;

	ItemStack item;
	try {
		item = ParseItemStack(*itemArg, count);
	} catch (...) {
		return ERROR_REASON_PARAMETERS;
	}

	if (!Items::IsValidId(item.id))
		return std::to_string(item.id) + " is not a valid item id!";

	SendChat(*ctx.session, "§eGave " + IdToLabel(item.id) + " (" + std::to_string(item.id) + ":" +
	                           std::to_string(item.data) + ") x" + std::to_string(item.count) + " to " +
	                           ctx.session->username);

	if (ctx.session->inventory.PickupItem(item))
		PacketUtilities::SendInventory(*ctx.session, ctx.session->openWindowId, ctx.session->inventory);
	return "";
}

} // namespace

void RegisterGive(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("give")
	                            .describe("Give yourself a block or item")
	                            .op()
	                            .then(strategos::Node::string("item").executes(GiveItem).then(
	                                strategos::Node::integer("amount").executes(GiveItem))));
}
