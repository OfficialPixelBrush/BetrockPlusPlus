/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @brief Discord Gateway bot: chat bridge, slash commands, crash uploads.
 *
 * Requires D++ (dpp). Enable with -DDISCORD_INTEGRATION=ON and the vcpkg
 * "discord" feature. Bot needs Message Content Intent in the Developer Portal.
 */

#ifdef DISCORD_INTEGRATION
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

class Server;

class Discord {
public:
	Discord();
	~Discord();

	Discord(const Discord&) = delete;
	Discord& operator=(const Discord&) = delete;

	// guildId is optional; when set, slash commands register to that guild (instant).
	// adminRoleId gates privileged slash commands (e.g. /stop). Empty = deny those commands.
	void Init(const std::string& _token, const std::string& _channelId, const std::string& _guildId = "",
	          const std::string& _adminRoleId = "");
	void Shutdown(const std::string& _finalMessage = "");

	void SendMessage(const std::string& _message);
	void SendFile(const std::string& _filename, const std::string& _message = "");

	void SendMessageSync(const std::string& _message);
	void SendFileSync(const std::string& _filename, const std::string& _message = "");

	// Drain inbound Discord chat / slash-command work onto the server tick thread.
	void Drain(Server& _server);

private:
	struct InboundChat {
		std::string author;
		std::string content;
	};

	using ServerTask = std::function<void(Server&)>;

	void EnqueueServerTask(ServerTask _task);

	static std::string RemoveMinecraftFormatting(const std::string& _input);
	// Builds §9[name] §f… lines, each at most 119 bytes (prefix included).
	static std::vector<std::string> FormatDiscordChatLines(const std::string& _author, const std::string& _content);
	static void BroadcastDiscordChat(Server& _server, const std::string& _author, const std::string& _content);

	struct Impl;
	std::unique_ptr<Impl> impl;

	std::mutex mutex;
	std::queue<InboundChat> inboundChat;
	std::queue<ServerTask> serverTasks;

	std::string token;
	std::string channelId;
	std::string guildId;
	std::string adminRoleId;
	std::atomic<bool> initialized{ false };
	std::atomic<bool> shuttingDown{ false };
};

Discord& GlobalDiscord();

#endif
