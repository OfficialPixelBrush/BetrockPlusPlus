/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "addon_impl.h"
#include "addon_api.h"
#include "logger.h"

void LogInfo(const char* _message) {
	GlobalLogger().error << _message << "\n";
}

void LogWarning(const char* _message) {
	GlobalLogger().error << _message << "\n";
}

void LogError(const char* _message) {
	GlobalLogger().error << _message << "\n";
}

void PlayerSendMessage(bp_player* _player, const char* _message) {
	Packet::ChatMessage pak;
	pak.message = std::string(_message);
	pak.Serialize(_player->session.stream);
}

bp_api MakeAddonAPI() {
	return bp_api{ .version = ADDON_API_VERSION,
		           .log = { .info = LogInfo, .warning = LogWarning, .error = LogError },
		           .player = { .sendMessage = PlayerSendMessage } };
}