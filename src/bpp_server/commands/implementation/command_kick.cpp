/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "server.h"
#include "username.h"
#include <algorithm>
#include <format>

namespace {

std::string TargetName(const strategos::CmdNode& _cmd, CommandContext& _ctx) {
	if (auto name = _cmd.get_arg<std::string>("username"))
		return *name;
	return _ctx.session->username;
}

std::string KickPlayer(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	std::string name = TargetName(_cmd, ctx);
	if (!IsValidUsername(name))
		return "Invalid username!";
	auto target = ctx.server->GetSessionByUsername(name);
	if (!target)
		return std::format("{} is not connected!", name);
	ctx.server->DisconnectPlayer("Kicked by an operator", *target);
	// Don't send if sender kicked themselves
	if (target.get() != ctx.session)
		SendChat(*ctx.session, std::format("{} has been kicked!", name));
	return "";
}

} // namespace

void RegisterKick(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("kick")
	                            .describe("Forcefully disconnects the named player")
	                            .op()
	                            .executes(KickPlayer)
	                            .then(strategos::Node::string("username").executes(KickPlayer)));
}
