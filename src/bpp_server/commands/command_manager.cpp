/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "command_manager.h"
#include "command.h"
#include "logger.h"

std::vector<std::unique_ptr<Command>> CommandManager::registeredCommands;
Server* CommandManager::m_server = nullptr;

// Register all commands
void CommandManager::Init(Server* server) {
	m_server = server;
	// Anyone can run these
	registeredCommands.push_back(std::make_unique<CommandHelp>());
	registeredCommands.push_back(std::make_unique<CommandTeleport>());
	registeredCommands.push_back(std::make_unique<CommandTime>());
	registeredCommands.push_back(std::make_unique<CommandSeed>());
	registeredCommands.push_back(std::make_unique<CommandSpawn>());
	registeredCommands.push_back(std::make_unique<CommandGive>());
	registeredCommands.push_back(std::make_unique<CommandList>());
	registeredCommands.push_back(std::make_unique<CommandLoaded>());
	registeredCommands.push_back(std::make_unique<CommandDimension>());
	registeredCommands.push_back(std::make_unique<CommandVersion>());
	registeredCommands.push_back(std::make_unique<CommandSummon>());
	registeredCommands.push_back(std::make_unique<CommandStats>());
	/*
	registeredCommands.push_back(CommandPose());
	// Needs at least creative mode to run
	registeredCommands.push_back(CommandHealth());
	// Must be operator
	registeredCommands.push_back(CommandUptime());
	registeredCommands.push_back(CommandOp());
	registeredCommands.push_back(CommandDeop());
	registeredCommands.push_back(CommandWhitelist());
	registeredCommands.push_back(CommandKick());
	registeredCommands.push_back(CommandCreative());
	registeredCommands.push_back(CommandSound());
	registeredCommands.push_back(CommandKill());
	registeredCommands.push_back(CommandGamerule());
	registeredCommands.push_back(CommandSave());
	registeredCommands.push_back(CommandStop());
	registeredCommands.push_back(CommandFree());
	registeredCommands.push_back(CommandUsage());
	registeredCommands.push_back(CommandSummon());
	registeredCommands.push_back(CommandPopulated());
	registeredCommands.push_back(CommandInterface());
	registeredCommands.push_back(CommandRegion());
	registeredCommands.push_back(CommandEntity());
	registeredCommands.push_back(CommandModified());
	registeredCommands.push_back(CommandPacket());
	*/
	GlobalLogger().m_info << "Registered " << registeredCommands.size() << " command(s)!" << "\n";
}

// Get all registered commands
const std::vector<std::unique_ptr<Command>>& CommandManager::GetRegisteredCommands() noexcept {
	return registeredCommands;
}

// Parses commands and executes them
void CommandManager::Parse(std::string& cmd_string, PlayerSession& session, WorldManager& world,
                           std::function<void(PlayerSession&)> transferDimension) noexcept {
	// Remove initial /
	cmd_string = cmd_string.substr(1);
	// Set these up for command parsing
	std::string failureReason = "Syntax";
	std::vector<std::string> command;

	std::string s;
	std::stringstream ss(cmd_string);

	while (getline(ss, s, ' ')) {
		// store token string in the vector
		command.push_back(s);
	}
	// No arguments passed, exit early
	if (command.empty() || cmd_string.empty()) {
		failureReason = ERROR_REASON_NO_CMD;
	} else {
		try {
			// TODO: Make this efficient
			for (size_t i = 0; i < registeredCommands.size(); i++) {
				// This'll throw an out of bounds error
				if (registeredCommands[i]->GetLabel() == command.at(0)) {
					failureReason = registeredCommands[i]->Execute(command, session, world, transferDimension,
					                                               *m_server);
					break;
				}
			}
		} catch (const std::exception& e) {
			GlobalLogger().m_info << e.what() << " on /" << cmd_string << "\n";
		}
	}

	Packet::ChatMessage failPkt;
	if (!failureReason.empty()) {
		if (failureReason == "Syntax")
			failPkt.m_message = "§cInvalid Syntax \"" + cmd_string + "\"";
		else
			failPkt.m_message = "§c" + failureReason;
		failPkt.Serialize(session.m_stream);
	}
}