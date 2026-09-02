/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"

namespace {

std::string ShowLoaded(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	SendChat(*ctx.session, "§e" + std::to_string(ctx.world->chunks.size()));
	return "";
}

} // namespace

void RegisterLoaded(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(
	    strategos::Node::literal("loaded").describe("Shows the number of loaded chunks").op().executes(ShowLoaded));
}
