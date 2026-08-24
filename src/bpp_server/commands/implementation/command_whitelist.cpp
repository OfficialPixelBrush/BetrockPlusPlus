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

// Adjust the servers whitelist/allowlist
// Usage:
//   /whitelist <add/remove/list/reload/on/off> [username]
std::string CommandWhitelist::Execute(std::vector<std::string>& _parameters, PlayerSession& _session,
                                      [[maybe_unused]] WorldManager& _world,
                                      [[maybe_unused]] std::function<void(PlayerSession&)> _transferDimension,
                                      Server& _server) {
	if (_parameters.size() < 2)
		return ERROR_REASON_TOO_FEW_PARAMETERS;

	const bool needsUsername = _parameters[1] == "add" || _parameters[1] == "remove";
	if (needsUsername && _parameters.size() < 3)
		return ERROR_REASON_TOO_FEW_PARAMETERS;

	std::string targetName;
	if (_parameters.size() >= 3)
		targetName = _parameters[2];

	if (_parameters[1] == "add") {
		// Check if already in whitelist
		auto it = std::find(_server.whitelistedUsernames.begin(), _server.whitelistedUsernames.end(), targetName);
		if (it != _server.whitelistedUsernames.end())
			return "User is already on whitelist!";
		_server.whitelistedUsernames.push_back(targetName);
		Packet::ChatMessage msg;
		msg.message = std::format("{} has been added to the whitelist!", targetName);
		msg.Serialize(_session.stream);
		return "";
	}
	if (_parameters[1] == "remove") {
		auto it = std::find(_server.whitelistedUsernames.begin(), _server.whitelistedUsernames.end(), targetName);
		if (it != _server.whitelistedUsernames.end())
			_server.whitelistedUsernames.erase(it);
		Packet::ChatMessage msg;
		msg.message = std::format("{} has been removed from the whitelist!", targetName);
		msg.Serialize(_session.stream);
		return "";
	}
	if (_parameters[1] == "on") {
		_server.useWhitelist = true;
		Packet::ChatMessage msg;
		msg.message = "Whitelist has been enabled!";
		msg.Serialize(_session.stream);
		return "";
	}
	if (_parameters[1] == "off") {
		_server.useWhitelist = false;
		Packet::ChatMessage msg;
		msg.message = "Whitelist has been disabled!";
		msg.Serialize(_session.stream);
		return "";
	}
	if (_parameters[1] == "list") {
		Packet::ChatMessage pkt;
		pkt.message = std::format("§7-- {} Whitelisted Player(s) --", _server.whitelistedUsernames.size());
		pkt.Serialize(_session.stream);
		pkt.message = "§7";
		for (size_t i = 0; i < _server.whitelistedUsernames.size(); i++) {
			auto& p = _server.whitelistedUsernames[i];
			if (pkt.message.size() + p.size() > 64) {
				pkt.Serialize(_session.stream);
				pkt.message = "§7";
			}
			pkt.message += p + ((i < (_server.whitelistedUsernames.size() - 1)) ? ", " : "");
		}
		pkt.Serialize(_session.stream);
		return "";
	}
	if (_parameters[1] == "reload") {
		_server.whitelistedUsernames = ListParser::Read(ListParser::Target::Whitelist);
		Packet::ChatMessage pkt;
		pkt.message = "Reloaded whitelist!";
		pkt.Serialize(_session.stream);
		return "";
	}
	return ERROR_REASON_PARAMETERS;
}
