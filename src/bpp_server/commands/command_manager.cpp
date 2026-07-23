/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "command_manager.h"
#include "command.h"
#include "logger.h"

std::vector<std::unique_ptr<Command>> CommandManager::m_registeredCommands;
Server* CommandManager::m_server = nullptr;

// Register all commands
void CommandManager::Init(Server* _server) {
	m_server = _server;
	// Anyone can run these
	m_registeredCommands.push_back(std::make_unique<CommandHelp>());
	m_registeredCommands.push_back(std::make_unique<CommandTeleport>());
	m_registeredCommands.push_back(std::make_unique<CommandTime>());
	m_registeredCommands.push_back(std::make_unique<CommandSeed>());
	m_registeredCommands.push_back(std::make_unique<CommandSpawn>());
	m_registeredCommands.push_back(std::make_unique<CommandGive>());
	m_registeredCommands.push_back(std::make_unique<CommandList>());
	m_registeredCommands.push_back(std::make_unique<CommandLoaded>());
	m_registeredCommands.push_back(std::make_unique<CommandDimension>());
	m_registeredCommands.push_back(std::make_unique<CommandVersion>());
	m_registeredCommands.push_back(std::make_unique<CommandSummon>());
	m_registeredCommands.push_back(std::make_unique<CommandStats>());
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
	GlobalLogger().info << "Registered " << m_registeredCommands.size() << " command(s)!" << "\n";
}

// Get all registered commands
const std::vector<std::unique_ptr<Command>>& CommandManager::GetRegisteredCommands() noexcept {
	return m_registeredCommands;
}

// Parses commands and executes them
void CommandManager::Parse(std::string& _cmdString, PlayerSession& _session, WorldManager& _world,
                           std::function<void(PlayerSession&)> _transferDimension) noexcept {
	// Remove initial /
	_cmdString = _cmdString.substr(1);
	// Set these up for command parsing
	std::string failureReason = "Syntax";
	std::vector<std::string> command;

	std::string s;
	std::stringstream ss(_cmdString);

	while (getline(ss, s, ' ')) {
		// store token string in the vector
		command.push_back(s);
	}
	// No arguments passed, exit early
	if (command.empty() || _cmdString.empty()) {
		failureReason = ERROR_REASON_NO_CMD;
	} else {
		try {
			// TODO: Make this efficient
			for (size_t i = 0; i < m_registeredCommands.size(); i++) {
				// This'll throw an out of bounds error
				if (m_registeredCommands[i]->GetLabel() == command.at(0)) {
					failureReason = m_registeredCommands[i]->Execute(command, _session, _world, _transferDimension,
					                                                 *m_server);
					break;
				}
			}
		} catch (const std::exception& e) {
			GlobalLogger().info << e.what() << " on /" << _cmdString << "\n";
		}
	}

	Packet::ChatMessage failPkt;
	if (!failureReason.empty()) {
		if (failureReason == "Syntax")
			failPkt.message = "§cInvalid Syntax \"" + _cmdString + "\"";
		else
			failPkt.message = "§c" + failureReason;
		failPkt.Serialize(_session.stream);
	}
}