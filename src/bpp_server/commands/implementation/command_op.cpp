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

std::string OpPlayer(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	std::string name = TargetName(_cmd, ctx);
	if (!IsValidUsername(name))
		return "Invalid username!";
	auto it = std::find(ctx.server->operatorUsernames.begin(), ctx.server->operatorUsernames.end(), name);
	if (it != ctx.server->operatorUsernames.end())
		return "User is already an operator!";
	ctx.server->operatorUsernames.push_back(name);
	if (!ctx.server->SaveOperators())
		return "Failed to save ops!";
	SendChat(*ctx.session, std::format("{} has been opped!", name));
	return "";
}

std::string DeopPlayer(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	std::string name = TargetName(_cmd, ctx);
	auto it = std::find(ctx.server->operatorUsernames.begin(), ctx.server->operatorUsernames.end(), name);
	if (it == ctx.server->operatorUsernames.end())
		return "User is not an operator!";
	ctx.server->operatorUsernames.erase(it);
	if (!ctx.server->SaveOperators())
		return "Failed to save ops!";
	SendChat(*ctx.session, std::format("{} has been deopped!", name));
	return "";
}

} // namespace

void RegisterOp(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("op")
	                            .describe("Grants a players operator privilidges")
	                            .op()
	                            .executes(OpPlayer)
	                            .then(strategos::Node::string("username").executes(OpPlayer)));
	_dispatcher.add_command(strategos::Node::literal("deop")
	                            .describe("Revokes a players operator privilidges")
	                            .op()
	                            .executes(DeopPlayer)
	                            .then(strategos::Node::string("username").executes(DeopPlayer)));
}
