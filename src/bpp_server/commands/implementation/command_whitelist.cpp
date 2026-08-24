/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "config/list_parser.h"
#include "server.h"
#include <algorithm>
#include <format>

namespace {

std::string WhitelistAdd(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto name = _cmd.get_arg<std::string>("username");
	if (!name)
		return ERROR_REASON_TOO_FEW_PARAMETERS;
	auto it = std::find(ctx.server->whitelistedUsernames.begin(), ctx.server->whitelistedUsernames.end(), *name);
	if (it != ctx.server->whitelistedUsernames.end())
		return "User is already on whitelist!";
	ctx.server->whitelistedUsernames.push_back(*name);
	SendChat(*ctx.session, std::format("{} has been added to the whitelist!", *name));
	return "";
}

std::string WhitelistRemove(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto name = _cmd.get_arg<std::string>("username");
	if (!name)
		return ERROR_REASON_TOO_FEW_PARAMETERS;
	auto it = std::find(ctx.server->whitelistedUsernames.begin(), ctx.server->whitelistedUsernames.end(), *name);
	if (it != ctx.server->whitelistedUsernames.end())
		ctx.server->whitelistedUsernames.erase(it);
	SendChat(*ctx.session, std::format("{} has been removed from the whitelist!", *name));
	return "";
}

std::string WhitelistOn(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	ctx.server->useWhitelist = true;
	SendChat(*ctx.session, "Whitelist has been enabled!");
	return "";
}

std::string WhitelistOff(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	ctx.server->useWhitelist = false;
	SendChat(*ctx.session, "Whitelist has been disabled!");
	return "";
}

std::string WhitelistList(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	const auto& list = ctx.server->whitelistedUsernames;
	SendChat(*ctx.session, std::format("§7-- {} Whitelisted Player(s) --", list.size()));
	std::string line = "§7";
	for (size_t i = 0; i < list.size(); i++) {
		auto& p = list[i];
		if (line.size() + p.size() > 64) {
			SendChat(*ctx.session, line);
			line = "§7";
		}
		line += p + ((i < (list.size() - 1)) ? ", " : "");
	}
	SendChat(*ctx.session, line);
	return "";
}

std::string WhitelistReload(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	ctx.server->whitelistedUsernames = ListParser::Read(ListParser::Target::Whitelist);
	SendChat(*ctx.session, "Reloaded whitelist!");
	return "";
}

} // namespace

void RegisterWhitelist(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(
	    strategos::Node::literal("whitelist")
	        .describe("Adjust the servers whitelist/allowlist")
	        .op()
	        .then(strategos::Node::literal("add").then(strategos::Node::string("username").executes(WhitelistAdd)))
	        .then(strategos::Node::literal("remove").then(strategos::Node::string("username").executes(WhitelistRemove)))
	        .then(strategos::Node::literal("list").executes(WhitelistList))
	        .then(strategos::Node::literal("reload").executes(WhitelistReload))
	        .then(strategos::Node::literal("on").executes(WhitelistOn))
	        .then(strategos::Node::literal("off").executes(WhitelistOff)));
}
