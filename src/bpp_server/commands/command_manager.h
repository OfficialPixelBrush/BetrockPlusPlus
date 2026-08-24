/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include "../player_conn/player_session.h"
#include "strategos.h"
#include "world.h"
#include <functional>

class Server;

struct CommandContext {
	PlayerSession* session = nullptr;
	WorldManager* world = nullptr;
	Server* server = nullptr;
	std::function<void(PlayerSession&)> transferDimension;
	const strategos::BrigadierContext* dispatcher = nullptr;
};

inline CommandContext& CmdCtx(void* _userData) {
	return *static_cast<CommandContext*>(_userData);
}

class CommandManager {
public:
	void Init(Server* _server);
	void Parse(std::string& _cmdString, PlayerSession& _session, WorldManager& _world,
	           std::function<void(PlayerSession&)> _transferDimension) noexcept;
	const strategos::BrigadierContext& Dispatcher() const noexcept {
		return dispatcher;
	}

private:
	Server* server = nullptr;
	strategos::BrigadierContext dispatcher;
};
