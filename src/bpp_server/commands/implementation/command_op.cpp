/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../../packet/packet_utils.h"
#include "../../server.h"
#include "../command.h"
#include "../command_manager.h"
#include "inventory/item_stack.h"
#include "items.h"
#include "strings/labels.h"
#include <cstddef>
#include <string>

// Grants a players operator privilidges
// Usage:
//   /op [username]
std::string CommandOp::Execute(std::vector<std::string>& _parameters, PlayerSession& _session,
                               [[maybe_unused]] WorldManager& _world,
                               [[maybe_unused]] std::function<void(PlayerSession&)> _transferDimension,
                               Server& _server) {
	std::string targetName = _session.username;
	if (_parameters.size() >= 2) {
		targetName = _parameters[1];
	}
	// Check if already opped
	auto it = std::find(_server.operatorUsernames.begin(), _server.operatorUsernames.end(), targetName);
	if (it != _server.operatorUsernames.end())
		return "User is already an operator!";
	Packet::ChatMessage msg;
	_server.operatorUsernames.push_back(targetName);
	msg.message = std::format("{} has been opped!", targetName);
	msg.Serialize(_session.stream);
	return "";
}

// Revokes a players operator privilidges
// Usage:
//   /op [username]
std::string CommandDeop::Execute(std::vector<std::string>& _parameters, PlayerSession& _session,
                                 [[maybe_unused]] WorldManager& _world,
                                 [[maybe_unused]] std::function<void(PlayerSession&)> _transferDimension,
                                 Server& _server) {
	std::string& targetName = _session.username;
	if (_parameters.size() >= 2) {
		targetName = _parameters[1];
	}
	// Erase if found
	auto it = std::find(_server.operatorUsernames.begin(), _server.operatorUsernames.end(), targetName);
	if (it != _server.operatorUsernames.end())
		_server.operatorUsernames.erase(it);
	Packet::ChatMessage msg;
	msg.message = std::format("{} has been deopped!", targetName);
	msg.Serialize(_session.stream);
	return "";
}