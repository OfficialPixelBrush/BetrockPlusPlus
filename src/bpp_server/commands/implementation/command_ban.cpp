/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "server.h"
#include "username.h"
#include <algorithm>
#include <cctype>
#include <format>
#include <sstream>

namespace {

std::string TargetName(const strategos::CmdNode& _cmd, CommandContext& _ctx) {
	if (auto name = _cmd.get_arg<std::string>("username"))
		return *name;
	return _ctx.session->username;
}

bool IsValidIpAddress(const std::string& _ip) {
	std::stringstream stream(_ip);
	std::string segment;
	int segments = 0;
	while (std::getline(stream, segment, '.')) {
		segments++;
		if (segments > 4 || segment.empty() || segment.size() > 3)
			return false;
		for (unsigned char c : segment) {
			if (!std::isdigit(c))
				return false;
		}
		if (std::stoi(segment) > 255)
			return false;
	}
	return segments == 4;
}

void SendChunkedList(PlayerSession& _session, const std::string& _header, const std::vector<std::string>& _entries) {
	SendChat(_session, _header);
	std::string line = "§7";
	for (size_t i = 0; i < _entries.size(); i++) {
		auto& entry = _entries[i];
		const std::string suffix = (i < (_entries.size() - 1)) ? ", " : "";
		if (line.size() + entry.size() + suffix.size() > 64 && line != "§7") {
			SendChat(_session, line);
			line = "§7";
		}
		line += entry + suffix;
	}
	if (line != "§7")
		SendChat(_session, line);
}

std::string BanPlayer(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	std::string name = TargetName(_cmd, ctx);
	if (!IsValidUsername(name))
		return "Invalid username!";
	auto it = std::find(ctx.server->bannedUsernames.begin(), ctx.server->bannedUsernames.end(), name);
	if (it != ctx.server->bannedUsernames.end())
		return "User is already banned!";
	ctx.server->bannedUsernames.push_back(name);
	if (!ctx.server->SaveBannedPlayers())
		return "Failed to save banned players!";
	// Boot them immediately if they're currently online.
	if (auto target = ctx.server->GetSessionByUsername(name))
		ctx.server->DisconnectPlayer("You are banned from this server!", *target);
	SendChat(*ctx.session, std::format("{} has been banned!", name));
	return "";
}

std::string PardonPlayer(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	std::string name = TargetName(_cmd, ctx);
	auto it = std::find(ctx.server->bannedUsernames.begin(), ctx.server->bannedUsernames.end(), name);
	if (it == ctx.server->bannedUsernames.end())
		return "User is not banned!";
	ctx.server->bannedUsernames.erase(it);
	if (!ctx.server->SaveBannedPlayers())
		return "Failed to save banned players!";
	SendChat(*ctx.session, std::format("{} has been pardoned!", name));
	return "";
}

std::string BanIp(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto arg = _cmd.get_arg<std::string>("target");
	if (!arg)
		return ERROR_REASON_TOO_FEW_PARAMETERS;
	std::string ip = *arg;
	// Allow the target to be either a raw IP, or the username of a currently connected player.
	if (!IsValidIpAddress(ip)) {
		auto target = ctx.server->GetSessionByUsername(ip);
		if (!target || target->ipAddress.empty())
			return "Not a valid IP, and no connected player by that name!";
		ip = target->ipAddress;
	}
	auto it = std::find(ctx.server->bannedIps.begin(), ctx.server->bannedIps.end(), ip);
	if (it != ctx.server->bannedIps.end())
		return "IP is already banned!";
	ctx.server->bannedIps.push_back(ip);
	if (!ctx.server->SaveBannedIps())
		return "Failed to save banned IPs!";
	// Boot anyone currently connected from this IP.
	for (auto& player : ctx.server->GetPlayers()) {
		if (player->ipAddress == ip)
			ctx.server->DisconnectPlayer("Your IP address is banned from this server!", *player);
	}
	SendChat(*ctx.session, std::format("{} has been IP banned!", ip));
	return "";
}

/*
std::string PardonIp(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto arg = _cmd.get_arg<std::string>("ip");
	if (!arg)
		return ERROR_REASON_TOO_FEW_PARAMETERS;
	if (!IsValidIpAddress(*arg))
		return "Invalid IP address!";
	auto it = std::find(ctx.server->bannedIps.begin(), ctx.server->bannedIps.end(), *arg);
	if (it == ctx.server->bannedIps.end())
		return "IP is not banned!";
	ctx.server->bannedIps.erase(it);
	if (!ctx.server->SaveBannedIps())
		return "Failed to save banned IPs!";
	SendChat(*ctx.session, std::format("{} has been un-banned!", *arg));
	return "";
}
*/

std::string BanListPlayers(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	const auto& names = ctx.server->bannedUsernames;
	SendChunkedList(*ctx.session, std::format("§7-- {} Banned Player(s) --", names.size()), names);
	return "";
}

std::string BanListIps(const strategos::CmdNode&, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	const auto& ips = ctx.server->bannedIps;
	SendChunkedList(*ctx.session, std::format("§7-- {} Banned IP(s) --", ips.size()), ips);
	return "";
}

} // namespace

void RegisterBan(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("ban")
	                            .describe("Bans a player from the server")
	                            .op()
	                            .executes(BanPlayer)
	                            .then(strategos::Node::string("username").executes(BanPlayer)));
	_dispatcher.add_command(strategos::Node::literal("pardon")
	                            .describe("Removes a player from the ban list")
	                            .op()
	                            .executes(PardonPlayer)
	                            .then(strategos::Node::string("username").executes(PardonPlayer)));
	_dispatcher.add_command(strategos::Node::literal("ban-ip")
	                            .describe("Bans an IP address, or a connected player's IP, from the server")
	                            .op()
	                            .then(strategos::Node::string("target").executes(BanIp)));
	/*
	_dispatcher.add_command(strategos::Node::literal("pardon-ip")
	                            .describe("Removes an IP address from the ban list")
	                            .op()
	                            .then(strategos::Node::string("ip").executes(PardonIp)));
	*/
	_dispatcher.add_command(strategos::Node::literal("banlist")
	                            .describe("Lists banned players or IP addresses")
	                            .op()
	                            .executes(BanListPlayers)
	                            .then(strategos::Node::literal("players").executes(BanListPlayers))
	                            .then(strategos::Node::literal("ips").executes(BanListIps)));
}
