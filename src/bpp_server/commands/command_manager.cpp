/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#include "command_manager.h"
#include "command.h"

std::vector<std::unique_ptr<Command>> CommandManager::registeredCommands;

// Register all commands
void CommandManager::Init() {
	// Anyone can run these
	registeredCommands.push_back(std::make_unique<CommandHelp>());
	registeredCommands.push_back(std::make_unique<CommandTeleport>());
	/*
	registeredCommands.push_back(CommandVersion());
	registeredCommands.push_back(CommandList());
	registeredCommands.push_back(CommandPose());
	registeredCommands.push_back(CommandSpawn());
	// Needs at least creative mode to run
	registeredCommands.push_back(CommandTeleport());
	registeredCommands.push_back(CommandGive());
	registeredCommands.push_back(CommandHealth());
	// Must be operator
	registeredCommands.push_back(CommandUptime());
	registeredCommands.push_back(CommandTime());
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
	registeredCommands.push_back(CommandLoaded());
	registeredCommands.push_back(CommandUsage());
	registeredCommands.push_back(CommandSummon());
	registeredCommands.push_back(CommandPopulated());
	registeredCommands.push_back(CommandInterface());
	registeredCommands.push_back(CommandRegion());
	registeredCommands.push_back(CommandSeed());
	registeredCommands.push_back(CommandEntity());
	registeredCommands.push_back(CommandModified());
	registeredCommands.push_back(CommandPacket());
	*/
	std::cout << "Registered " << registeredCommands.size() << " command(s)!" << "\n";
}

// Get all registered commands
const std::vector<std::unique_ptr<Command>>& CommandManager::GetRegisteredCommands() noexcept { return registeredCommands; }

// Parses commands and executes them
void CommandManager::Parse(std::wstring& cmd_string, PlayerSession& session) noexcept {
	// Remove initial /
	cmd_string = cmd_string.substr(1);
	// Set these up for command parsing
	std::wstring failureReason = L"Syntax";
	std::vector<std::wstring> command;

	std::wstring s;
	std::wstringstream ss(cmd_string);

	while (getline(ss, s, L' ')) {
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
					failureReason = registeredCommands[i]->Execute(command, session);
					break;
				}
			}
		}
		catch (const std::exception& e) {
			//Betrock::Logger::Instance().Info(std::string(e.what()) + std::string(" on /") + cmd_string);
		}
	}

	Packet::ChatMessage failPkt;

	if (failureReason == L"Syntax") {
		failPkt.message = L"§cInvalid Syntax \"" + cmd_string + L"\"";
	}
	else if (!failureReason.empty()) {
		failPkt.message = L"§c" + failureReason;
	}
	failPkt.Serialize(session.stream);
}