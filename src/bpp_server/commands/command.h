/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Mojang Studios (2011)
*/

#pragma once
#include <sstream>

#include "../player_session.h"

#define ERROR_OPERATOR "Only operators can use this command!"
#define ERROR_CREATIVE "Only creative players can use this command!"
#define ERROR_WHITELIST "Only whitelisted players can use this command!"
#define ERROR_REASON_SYNTAX "Invalid Syntax"
#define ERROR_REASON_PARAMETERS "Invalid Parameters"
#define ERROR_REASON_ERROR "Error"

#define MAX_CHAT_LINE_SIZE 60

// Small defins for a bit less copy-paste
#define DEFINE_COMMAND(name, label, description, syntax, requiresOp, requiresCreative)                                 \
	class name : public Command {                                                                                      \
	  public:                                                                                                          \
		name() : Command(label, description, syntax, requiresOp, requiresCreative) {}                                  \
		std::string Execute(std::vector<std::string>& parameters, PlayerSession& session) override;                                                                  \
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
	std::string label;
	std::string description;
	std::string syntax;
	bool requiresOp;
	bool requiresCreative;

  public:
	std::string GetLabel() { return label; }
	std::string GetDescription() { return description; }
	std::string GetSyntax() { return syntax; }
	bool GetRequiresOperator() { return requiresOp; }
	bool GetRequiresCreative() { return requiresCreative; }

	std::string CheckPermissions(PlayerSession& session);
	Command(std::string label, std::string description, std::string syntax, bool requiresOp = true, bool requiresCreative = false);
	virtual std::string Execute(std::vector<std::string>& parameters, PlayerSession& session) = 0;
	virtual ~Command() = default;
};

// Commands
// Anyone can run these
DEFINE_COMMAND(CommandHelp, "help", "Lists commands or helps with command", "[command]", false, false);
/*
DEFINE_COMMAND(CommandVersion, "version", "Shows the current Server version", "", false, false);
DEFINE_COMMAND(CommandList, "list", "List all currently online players", "", false, false);
DEFINE_COMMAND(CommandPose, "pose", "Set the current players' pose", "<crouch/fire/sit>", false, false);
DEFINE_COMMAND(CommandSpawn, "spawn", "Teleport to Spawn", "", false, false);
DEFINE_COMMAND(CommandInterface, "interface", "Open the desired interface", "<id>", false, false);
// Needs at least creative mode to run
DEFINE_COMMAND(CommandTeleport, "tp", "Teleports player to coordinates or another player",
			   "<player> <x> <y> <z> <pitch> <yaw>/<player> <player>", false, true);
DEFINE_COMMAND(CommandGive, "give", "Give yourself a block or item", "<id> [meta] [amount]", false, true);
DEFINE_COMMAND(CommandHealth, "health", "Get or Set Player Health", "[health]", false, true);
// Must be operator
DEFINE_COMMAND(CommandUptime, "uptime", "Shows how long the server has been alive for in ticks", "", true, false);
DEFINE_COMMAND(CommandTime, "time", "Gets or sets the current world time", "[newtime]", true, false);
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
DEFINE_COMMAND(CommandSeed, "seed", "Get the world seed", "", true, false);
DEFINE_COMMAND(CommandEntity, "entity", "Get the latest entity id", "", true, false);
DEFINE_COMMAND(CommandModified, "modified", "Get the number of modified chunks", "", true, false);
DEFINE_COMMAND(CommandPacket, "packet", "Send a custom packet", "[broadcast] <data>", true, false);
*/