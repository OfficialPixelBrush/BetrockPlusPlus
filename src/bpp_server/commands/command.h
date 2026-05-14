/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#pragma once
#include <sstream>

#include "../player_session.h"
#include "world.h"

#define ERROR_OPERATOR L"Only operators can use this command!"
#define ERROR_CREATIVE L"Only creative players can use this command!"
#define ERROR_WHITELIST L"Only whitelisted players can use this command!"
#define ERROR_REASON_SYNTAX L"Invalid Syntax"
#define ERROR_REASON_PARAMETERS L"Invalid Parameters"
#define ERROR_REASON_ERROR L"Error"
#define ERROR_REASON_NO_CMD L"No command passed"

#define MAX_CHAT_LINE_SIZE 60

// Small defins for a bit less copy-paste
#define DEFINE_COMMAND(name, label, description, syntax, requiresOp, requiresCreative)                                 \
	class name : public Command {                                                                                      \
	  public:                                                                                                          \
		name() : Command(label, description, syntax, requiresOp, requiresCreative) {}                                  \
		std::wstring Execute(std::vector<std::wstring>& parameters, PlayerSession& session, WorldManager& world) override;                                                                  \
	};

/*
#define DEFINE_PERMSCHECK(session)                                                                                      \
	std::string perms = CheckPermissions(client);                                                                      \
	if (!perms.empty())                                                                                                \
		return perms;
*/

class CommandManager;

// Base class for how a command is defined
class Command {
private:
	std::wstring label;
	std::wstring description;
	std::wstring syntax;
	bool requiresOp;
	bool requiresCreative;

public:
	std::wstring GetLabel() { return label; }
	std::wstring GetDescription() { return description; }
	std::wstring GetSyntax() { return syntax; }
	bool GetRequiresOperator() { return requiresOp; }
	bool GetRequiresCreative() { return requiresCreative; }

	std::string CheckPermissions(PlayerSession& session);
	Command(std::wstring label, std::wstring description, std::wstring syntax, bool requiresOp = true, bool requiresCreative = false);
	virtual std::wstring Execute(std::vector<std::wstring>& parameters, PlayerSession& session, WorldManager& world) = 0;
	virtual ~Command() = default;
};

// Commands
// Anyone can run these
DEFINE_COMMAND(CommandHelp, L"help", L"Lists commands or helps with command", L"[command]", false, false);
DEFINE_COMMAND(CommandTeleport, L"tp", L"Teleports player to coordinates or another player",
	L"<player> <x> <y> <z> / <player> <player>", false, false);
DEFINE_COMMAND(CommandTime, L"time", L"Gets or sets the current world time", L"<new_time>", false, false);
DEFINE_COMMAND(CommandSpawn, L"spawn", L"Teleport to spawn", L"", false, false);
DEFINE_COMMAND(CommandSeed, L"seed", L"Get the world seed", L"", false, false);
//DEFINE_COMMAND(CommandGive, L"give", L"Give yourself a block or item", L"<id>:[meta] [amount]", false, false);
//DEFINE_COMMAND(CommandList, L"list", L"List all currently online players", L"", false, false);
//DEFINE_COMMAND(CommandLoaded, L"loaded", L"Shows the number of loaded chunks", L"", false, false);
/*
DEFINE_COMMAND(CommandVersion, "version", "Shows the current Server version", "", false, false);
DEFINE_COMMAND(CommandPose, "pose", "Set the current players' pose", "<crouch/fire/sit>", false, false);
DEFINE_COMMAND(CommandInterface, "interface", "Open the desired interface", "<id>", false, false);
// Needs at least creative mode to run
DEFINE_COMMAND(CommandGive, "give", "Give yourself a block or item", "<id> [meta] [amount]", false, true);
DEFINE_COMMAND(CommandHealth, "health", "Get or Set Player Health", "[health]", false, true);
// Must be operator
DEFINE_COMMAND(CommandUptime, "uptime", "Shows how long the server has been alive for in ticks", "", true, false);
DEFINE_COMMAND(CommandOp, "op", "Grant a player operator privlidges", "[player]", true, false);
DEFINE_COMMAND(CommandDeop, "deop", "Revoke a players' operator privlidges", "[player]", true, false);
DEFINE_COMMAND(CommandWhitelist, "whitelist", "Modify the whitelist", "<reload/list> / <add/remove> <player>", true,
			   false);
DEFINE_COMMAND(CommandKick, "kick", "Kick a player from the server", "[player]", true, false);
DEFINE_COMMAND(CommandCreative, "creative", "Toggle creative mode", "", true, false);
DEFINE_COMMAND(CommandSound, "sound", "Play a specified sound", "<id> [meta]", true, false);
DEFINE_COMMAND(CommandKill, "kill", "Kill the specified player", "[player]", true, false);
DEFINE_COMMAND(CommandGamerule, "gamerule", "Configure gamerules", "<rule> <state>", true, false);
DEFINE_COMMAND(CommandSave, "save", "Forces the server to save all loaded chunks", "", true, false);
DEFINE_COMMAND(CommandStop, "stop", "Forces the server to stop", "", true, false);
DEFINE_COMMAND(CommandFree, "free", "Forces the server to unload chunks nobody can see", "", true, false);
DEFINE_COMMAND(CommandLoaded, "loaded", "Shows the number of loaded chunks", "", true, false);
DEFINE_COMMAND(CommandUsage, "usage", "Shows the current memory usage in megabytes", "", true, false);
DEFINE_COMMAND(CommandSummon, "summon", "Summon a player entity", "<player>", true, false);
DEFINE_COMMAND(CommandPopulated, "populated", "Check the population status of the current chunk", "", true, false);
DEFINE_COMMAND(CommandRegion, "region", "Test the region infrastructure", "<action>", true, false);
DEFINE_COMMAND(CommandEntity, "entity", "Get the latest entity id", "", true, false);
DEFINE_COMMAND(CommandModified, "modified", "Get the number of modified chunks", "", true, false);
DEFINE_COMMAND(CommandPacket, "packet", "Send a custom packet", "[broadcast] <data>", true, false);
*/

// Helper: send a PlayerPositionAndRotation packet to move a session to new coords.
static void SendTeleport(PlayerSession& target, Double3 position, float yaw = 0.0f, float pitch = 0.0f) {
	Packet::PlayerPositionAndRotation pkt;
	pkt.x = position.x;
	pkt.y = position.y;
	pkt.stance = position.y + PLAYER_EYE_HEIGHT;
	pkt.z = position.z;
	pkt.yaw = yaw;
	pkt.pitch = pitch;
	pkt.onGround = false;
	pkt.Serialize(target.stream);
	// Keep server-side position in sync so movement broadcasts are correct.
	target.position.pos = position;
	target.lastFpX = static_cast<int32_t>(position.x * 32.0);
	target.lastFpY = static_cast<int32_t>(position.y * 32.0);
	target.lastFpZ = static_cast<int32_t>(position.z * 32.0);
	target.lastYaw = static_cast<int8_t>(yaw / 360.0f * 256.0f);
	target.lastPitch = static_cast<int8_t>(pitch / 360.0f * 256.0f);
}

// Helper: find a playing session by username.
static PlayerSession* FindSession(PlayerSession& caller, const std::wstring& name) {
	if (!caller.players) return nullptr;
	for (auto& s : *caller.players) {
		if (s->username == name && s->connState == ConnectionState::Playing)
			return s.get();
	}
	return nullptr;
}

inline Int3 ParseInt3(size_t& offset, std::vector<std::wstring>& parameters) {
	return Int3{
		std::stoi(parameters[offset++]),
		std::stoi(parameters[offset++]),
		std::stoi(parameters[offset++]),
	};
}

inline Float2 ParseFloat2(size_t& offset, std::vector<std::wstring>& parameters) {
	return Float2{
		std::stof(parameters[offset++]),
		std::stof(parameters[offset++]),
	};
}

inline Float3 ParseFloat3(size_t& offset, std::vector<std::wstring>& parameters) {
	return Float3{
		std::stof(parameters[offset++]),
		std::stof(parameters[offset++]),
		std::stof(parameters[offset++]),
	};
}

inline Double2 ParseDouble2(size_t& offset, std::vector<std::wstring>& parameters) {
	return Double2{
		std::stod(parameters[offset++]),
		std::stod(parameters[offset++]),
	};
}

inline Double3 ParseDouble3(size_t& offset, std::vector<std::wstring>& parameters) {
	return Double3{
		std::stod(parameters[offset++]),
		std::stod(parameters[offset++]),
		std::stod(parameters[offset++]),
	};
}