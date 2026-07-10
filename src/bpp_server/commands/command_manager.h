/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include "../player_conn/player_session.h"
#include "command.h"
#include <functional>

struct Server;

/**
 * @brief Responsible for all command handling and execution
 * 
 */
class CommandManager {
public:
	static void Init(Server* server);
	static void Parse(std::string& cmd_string, PlayerSession& session, WorldManager& world,
	                  std::function<void(PlayerSession&)> transferDimension) noexcept;
	static const std::vector<std::unique_ptr<Command>>& GetRegisteredCommands() noexcept;

private:
	static Server* m_server;
	static std::vector<std::unique_ptr<Command>> registeredCommands;
};