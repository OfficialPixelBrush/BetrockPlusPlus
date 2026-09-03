/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "command_manager.h"
#include "command.h"
#include "command_registry.h"
#include "logger.h"
#include <string_view>

void CommandManager::Init(Server* _server) {
	server = _server;
	RegisterHelp(dispatcher);
	RegisterTeleport(dispatcher);
	RegisterTime(dispatcher);
	RegisterSeed(dispatcher);
	RegisterSpawn(dispatcher);
	RegisterGive(dispatcher);
	RegisterList(dispatcher);
	RegisterLoaded(dispatcher);
	RegisterDimension(dispatcher);
	RegisterVersion(dispatcher);
	RegisterSummon(dispatcher);
	RegisterStats(dispatcher);
	RegisterFill(dispatcher);
	RegisterStop(dispatcher);
	RegisterOp(dispatcher);
	RegisterWhitelist(dispatcher);
	RegisterKick(dispatcher);
	RegisterBan(dispatcher);
	GlobalLogger().info << "Registered " << dispatcher.root().children.size() << " command(s)!" << "\n";
}

void CommandManager::Parse(std::string& _cmdString, PlayerSession& _session, WorldManager& _world,
                           std::function<void(PlayerSession&)> _transferDimension) noexcept {
	std::string_view line = _cmdString;
	if (!line.empty() && line.front() == '/')
		line.remove_prefix(1);

	if (line.empty()) {
		SendChat(_session, "§c" ERROR_REASON_NO_CMD);
		return;
	}

	CommandContext ctx{ &_session, &_world, server, std::move(_transferDimension), &dispatcher };

	try {
		auto result = dispatcher.execute(line, &ctx, [](void* _userData) {
			auto& c = CmdCtx(_userData);
			return IsOperator(*c.session, *c.server);
		});
		if (!result) {
			const auto& err = result.error();
			std::string message = err.message;
			if (err.error == strategos::ParseError::NoPermission)
				message = ERROR_OPERATOR;
			else if (err.error == strategos::ParseError::UnknownCommand && err.position == 0)
				message = ERROR_REASON_NO_EXIST;
			SendChat(_session, "§c" + message);
			return;
		}
		if (!result->empty())
			SendChat(_session, "§c" + *result);
	} catch (const std::exception& e) {
		GlobalLogger().info << e.what() << " on /" << std::string(line) << "\n";
	}
}
