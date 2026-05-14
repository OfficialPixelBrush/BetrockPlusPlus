/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"

// Lists pCommands or helps with command
std::wstring CommandHelp::Execute(std::vector<std::wstring>& parameters, PlayerSession& session, WorldManager& world) {
	//DEFINE_PERMSCHECK(pClient)
	const auto& registered_commands = CommandManager::GetRegisteredCommands();
	Packet::ChatMessage pkt;
	// Get help with specific command
	if (parameters.size() > 1) {
		for (size_t i = 0; i < registered_commands.size(); i++) {
			if (registered_commands[i]->GetLabel() == parameters[1]) {
				pkt.message = L"§7" + registered_commands[i]->GetLabel() + L": " + registered_commands[i]->GetDescription();
				pkt.Serialize(session.stream);
				// Only print syntax if it has a value
				if (!registered_commands[i]->GetSyntax().empty()) {
					pkt.message = L"§7/" + registered_commands[i]->GetLabel() + L" " + registered_commands[i]->GetSyntax();
					pkt.Serialize(session.stream);
				}
				if (registered_commands[i]->GetRequiresOperator()) {
					pkt.message = L"§7(Requires operator)";
					pkt.Serialize(session.stream);
				}
				return L"";
			}
		}
		return L"Command not found!";
		// List all commands
	}
	else {
		pkt.message = L"§7-- All Commands --";
		pkt.Serialize(session.stream);
		pkt.message = L"§7";

		for (size_t i = 0; i < registered_commands.size(); i++) {
			pkt.message += registered_commands[i]->GetLabel();

			if (i < registered_commands.size() - 1) {
				pkt.message += L", ";
			}

			if (pkt.message.size() > MAX_CHAT_LINE_SIZE ||
				i == registered_commands.size() - 1) {
				pkt.Serialize(session.stream);
				pkt.message = L"§7";
			}
		}
		return L"";
	}
	return ERROR_REASON_SYNTAX;
}