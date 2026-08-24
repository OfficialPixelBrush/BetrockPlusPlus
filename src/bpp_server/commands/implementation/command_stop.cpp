/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "server.h"
#include <cstdint>
#include <format>

namespace {

std::string StopNow(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	ctx.server->SendGlobalChatMessage(std::format("§eStopping..."));
	shutdownRequested.store(true);
	return "";
}

std::string CancelStop(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	ctx.server->ResetTimeout();
	ctx.server->SendGlobalChatMessage(std::format("§eCancelled stop!"));
	return "";
}

std::string StopInSeconds(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto seconds = _cmd.get_arg<float>("seconds");
	if (!seconds)
		return ERROR_REASON_PARAMETERS;

	float timeout = *seconds;
	static constexpr float MAX_TIMEOUT = UINT16_MAX / Server::TICKS_PER_SECOND;
	if (timeout > MAX_TIMEOUT)
		return std::format("Exceeds max timeout! ({} seconds)", MAX_TIMEOUT);

	ctx.server->SendGlobalChatMessage(std::format("§eStopping in {:.1f} seconds...", timeout));
	ctx.server->StopTimeout(timeout);
	return "";
}

} // namespace

void RegisterStop(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("stop")
	                            .describe("Forces the server to stop")
	                            .op()
	                            .executes(StopNow)
	                            .then(strategos::Node::literal("cancel").executes(CancelStop))
	                            .then(strategos::Node::float_("seconds").executes(StopInSeconds)));
}
