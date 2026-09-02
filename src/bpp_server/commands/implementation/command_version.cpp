/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "version.h"

namespace {

std::string ShowVersion(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	SendChat(*ctx.session,
	         "§eCurrent " + std::string(PROJECT_NAME) + " version is " + std::string(PROJECT_VERSION_FULL_STRING));
	return "";
}

} // namespace

void RegisterVersion(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(
	    strategos::Node::literal("version").describe("Shows the current Server version").executes(ShowVersion));
}
