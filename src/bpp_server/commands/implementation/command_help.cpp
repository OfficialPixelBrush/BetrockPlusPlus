/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"

// Lists commands or helps with command
// Usage:
//   /help
//   /help [command]
std::string CommandHelp::Execute(std::vector<std::string>& _parameters, PlayerSession& _session, WorldManager& _world,
                                 [[maybe_unused]] std::function<void(PlayerSession&)> _transferDimension,
                                 [[maybe_unused]] Server& _server) {
	//DEFINE_PERMSCHECK(pClient)
	const auto& registeredCommands = CommandManager::GetRegisteredCommands();
	Packet::ChatMessage pkt;
	// Get help with specific command
	if (_parameters.size() > 1) {
		for (size_t i = 0; i < registeredCommands.size(); i++) {
			if (registeredCommands[i]->GetLabel() == _parameters[1]) {
				pkt.message = "§7" + registeredCommands[i]->GetLabel() + ": " +
				              registeredCommands[i]->GetDescription();
				pkt.Serialize(_session.stream);
				// Only print syntax if it has a value
				if (!registeredCommands[i]->GetSyntax().empty()) {
					pkt.message = "§7/" + registeredCommands[i]->GetLabel() + " " + registeredCommands[i]->GetSyntax();
					pkt.Serialize(_session.stream);
				}
				if (registeredCommands[i]->GetRequiresOperator()) {
					pkt.message = "§7(Requires operator)";
					pkt.Serialize(_session.stream);
				}
				return "";
			}
		}
		return "Command not found!";
	} else {
		// List all commands
		pkt.message = "§7-- All commands --";
		pkt.Serialize(_session.stream);
		pkt.message = "§7";
		for (size_t i = 0; i < registeredCommands.size(); i++) {
			pkt.message += registeredCommands[i]->GetLabel();
			if (i < registeredCommands.size() - 1) {
				pkt.message += ", ";
			}
			if (pkt.message.size() > MAX_CHAT_LINE_SIZE || i == registeredCommands.size() - 1) {
				pkt.Serialize(_session.stream);
				pkt.message = "§7";
			}
		}
		return "";
	}
	return ERROR_REASON_SYNTAX;
}