/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "server.h"

// Teleports a player to coordinates or to another player.
// Usage:
//   /tp <x> <y> <z>
//   /tp <player> <x> <y> <z>
//   /tp <source_player> <target_player>
std::string CommandTeleport::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                     std::function<void(PlayerSession&)> _transferDimension, Server& _server) {
	if (_parameters.size() < 2)
		return ERROR_REASON_SYNTAX;

	PlayerSession* source = nullptr;
	size_t offset = 1;

	// Check if player is even passed
	// Inspired by https://stackoverflow.com/a/16575564
	{
		std::stringstream ss;
		ss << _parameters[offset];
		double num = 0.0;
		ss >> num;
		if (!ss.fail() && ss.eof())
			source = &_session;
		else {
			source = _server.getSessionByUsername(_parameters[offset++]).get();
		}
	}

	// TODO Should prolly report if a non-existent player runs this
	if (!source)
		return _parameters[offset - 1] + " does not exist!";

	// /tp <player> <x> <y> <z>
	if (_parameters.size() - offset >= 3) {
		try {
			Vec3 pos = ParseDouble3(offset, _parameters);
			SendTeleport(*source, pos);

			Packet::ChatMessage reply;
			reply.message = "§eTeleported " + source->username + " to " + pos.str();
			reply.Serialize(_session.stream);
			return "";
		} catch (...) {
			return ERROR_REASON_PARAMETERS;
		}
	}

	// /tp <player> <target_player>
	if (_parameters.size() - offset == 1) { // offset=1→params[1], offset=2→params[2]
		PlayerSession* dest = _server.getSessionByUsername(_parameters[offset]).get();
		if (!dest)
			return _parameters[offset] + " does not exist!";
		SendTeleport(*source, dest->position.pos, dest->rotation.x, dest->rotation.y);
		Packet::ChatMessage reply;
		reply.message = "§eTeleported " + source->username + " to " + _session.username;
		reply.Serialize(_session.stream);
		return "";
	}

	return ERROR_REASON_SYNTAX;
}