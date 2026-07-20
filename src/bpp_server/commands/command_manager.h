/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include "../player_conn/player_session.h"
#include "command.h"
#include <functional>

class Server;

/**
 * @brief Responsible for all command handling and execution
 * 
 */
class CommandManager {
public:
	static void Init(Server* _server);
	static void Parse(std::string& _cmd_string, PlayerSession& _session, WorldManager& _world,
	                  std::function<void(PlayerSession&)> _transferDimension) noexcept;
	static const std::vector<std::unique_ptr<Command>>& GetRegisteredCommands() noexcept;

private:
	static Server* server;
	static std::vector<std::unique_ptr<Command>> registeredCommands;
};