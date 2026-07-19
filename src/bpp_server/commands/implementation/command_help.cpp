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
std::string CommandHelp::Execute(std::vector<std::string>& parameters, PlayerSession& session, WorldManager& world,
                                 [[maybe_unused]] std::function<void(PlayerSession&)> transferDimension,
                                 [[maybe_unused]] Server& server) {
	//DEFINE_PERMSCHECK(pClient)
	const auto& registered_commands = CommandManager::GetRegisteredCommands();
	Packet::ChatMessage pkt;
	// Get help with specific command
	if (parameters.size() > 1) {
		for (size_t i = 0; i < registered_commands.size(); i++) {
			if (registered_commands[i]->GetLabel() == parameters[1]) {
				pkt.m_message = "§7" + registered_commands[i]->GetLabel() + ": " +
				              registered_commands[i]->GetDescription();
				pkt.Serialize(session.m_stream);
				// Only print syntax if it has a value
				if (!registered_commands[i]->GetSyntax().empty()) {
					pkt.m_message = "§7/" + registered_commands[i]->GetLabel() + " " + registered_commands[i]->GetSyntax();
					pkt.Serialize(session.m_stream);
				}
				if (registered_commands[i]->GetRequiresOperator()) {
					pkt.m_message = "§7(Requires operator)";
					pkt.Serialize(session.m_stream);
				}
				return "";
			}
		}
		return "Command not found!";
	} else {
		// List all commands
		pkt.m_message = "§7-- All commands --";
		pkt.Serialize(session.m_stream);
		pkt.m_message = "§7";
		for (size_t i = 0; i < registered_commands.size(); i++) {
			pkt.m_message += registered_commands[i]->GetLabel();
			if (i < registered_commands.size() - 1) {
				pkt.m_message += ", ";
			}
			if (pkt.m_message.size() > MAX_CHAT_LINE_SIZE || i == registered_commands.size() - 1) {
				pkt.Serialize(session.m_stream);
				pkt.m_message = "§7";
			}
		}
		return "";
	}
	return ERROR_REASON_SYNTAX;
}