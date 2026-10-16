/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "version.h"
#include "gamerules.h"

namespace {

std::string SetGamerule(const strategos::CmdNode& _cmd, void* /*_userData*/) {
	//auto& ctx = CmdCtx(_userData);
	auto gameruleName = _cmd.get_arg<std::string>("gamerule");
	if (!gameruleName)
		return "Gamerule doesn't exist!";
	// TODO: Convert to needed type
	//Gamerule::Set(gameruleName)
	//SendFeedback(std::to_string())
	return "";
}
std::string GetGamerule(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto gameruleName = _cmd.get_arg<std::string>("gamerule");
	if (!gameruleName)
		return "Gamerule doesn't exist!";
	// TODO: Figure out needed type
	SendFeedback(ctx, std::to_string(Gamerule::Get<bool>(*gameruleName)));
	return "";
}
// TODO
std::string ListGamerules(const strategos::CmdNode& /*_cmd*/, void* /*_userData*/) {
	//auto& ctx = CmdCtx(_userData);
	//SendFeedback(ctx, std::to_string(Gamerule::Get<bool>(*gameruleName)));
	return "";
}

} // namespace


void RegisterGamerule(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(
	    strategos::Node::literal("gamerule")
	        .describe("Lists, queries, or changes gamerules")
	        .op()
	        .then(strategos::Node::literal("list").executes(ListGamerules))
	        .then(strategos::Node::string("gamerule")
	                  .executes(GetGamerule)
	                  .then(strategos::Node::string("value").executes(SetGamerule))));
}