/*
 * Copyright (c) 2026, Anya Rihtarshich <vesui@proton.me>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "server.h"

namespace {

std::string ExecuteAsPlayer(const strategos::CmdNode& command, void* userData) {
	auto& ctx = CmdCtx(userData);
	if (ctx.executionDepth >= 16) return "Maximum execute depth exceeded!";

	auto playerName = command.get_arg<std::string>("player");
	auto nestedCommand = command.get_arg<std::string>("command");

	if (!playerName || !nestedCommand) return ERROR_REASON_PARAMETERS;

	auto target = ctx.server->GetSessionByUsername(*playerName);
	if (!target || !target->stream.IsConnected() || target->connState != ConnectionState::Playing || !target->entity)
		return *playerName + " is not connected!";

	CommandContext nested = ctx;
	nested.session = target.get();
	nested.world = ctx.server->GetWorldForDimension(target->dimension);
	nested.executionDepth = ctx.executionDepth + 1;

	if (!ctx.manager->Parse(*nestedCommand, nested))
        ctx.executionSucceeded = false;

	return "";
}

} // namespace

void RegisterExecute(strategos::BrigadierContext& dispatcher) {
	dispatcher.add_command(strategos::Node::literal("execute")
	                           .describe("Runs a command as another player")
	                           .op()
	                           .then(strategos::Node::string("player")
	                                     .suggests(strategos::SuggestionSource::Players)
	                                     .then(strategos::Node::greedy_string("command")
	                                               .suggests(strategos::SuggestionSource::NestedCommand)
	                                               .executes(ExecuteAsPlayer))));
}
