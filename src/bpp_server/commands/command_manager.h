/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#pragma once
#include <functional>
#include "../player_session.h"
#include "command.h"

/**
 * @brief Responsible for all command handling and execution
 * 
 */
class CommandManager {
  public:
	static void Init();
	static void Parse(std::wstring &cmd_string, PlayerSession& session, WorldManager& world, std::function<void(PlayerSession&)> transferDimension) noexcept;
	static const std::vector<std::unique_ptr<Command>> &GetRegisteredCommands() noexcept;

  private:
	static std::vector<std::unique_ptr<Command>> registeredCommands;
};