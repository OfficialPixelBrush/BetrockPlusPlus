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
	enum class EmbedColor : int8_t {
		None,
		Red,
		Yellow,
		Green,
		Blue
	};
	Discord();
	~Discord();

	Discord(const Discord&) = delete;
	Discord& operator=(const Discord&) = delete;

	// guildId is optional; when set, slash commands register to that guild (instant).
	// adminRoleId gates privileged slash commands (e.g. /stop). Empty = deny those commands.
	// webhookUrl is optional; when set, SendPlayerChatMessage() posts through it so each
	// message appears under the player's own name + skin face instead of the bot's identity.
	void Init(const std::string& _token, const std::string& _channelId, const std::string& _guildId = "",
	          const std::string& _adminRoleId = "", const std::string& _webhookUrl = "");
	void Shutdown(const std::string& _finalMessage = "");

	void SendMessage(const std::string& _message);
	void SendFile(const std::string& _filename, const std::string& _message = "");

	void SendMessageSync(const std::string& _message);
	void SendFileSync(const std::string& _filename, const std::string& _message = "");

	void SendServerNotice(const std::string& _message, const EmbedColor _color = Discord::EmbedColor::None);
	void SendPlayerChatMessage(const std::string& _username, const std::string& _message);

	// Posts a join/leave notice as an embed with the player's skin face as the thumbnail.
	// Always sent as the bot (these are server events, not chat impersonation).
	void SendPlayerJoinMessage(const std::string& _username);
	void SendPlayerLeaveMessage(const std::string& _username);

	// Drain inbound Discord chat / slash-command work onto the server tick thread.
	void Drain(Server& _server);
	constexpr uint32_t GetColorCode(const EmbedColor _color) const;

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

	// Face-only skin render for _username, cropped server-side by the image host (Discord
	// fetches this URL itself when it renders the message; we never download or decode a
	// skin locally). _size is the output width/height in pixels.
	static std::string BuildSkinAvatarUrl(const std::string& _username, int _size);
	// Clamps to Discord's webhook username rules (1-80 chars, no "discord"/"clyde").
	static std::string SanitizeWebhookUsername(const std::string& _username);
	void SendPlayerEventEmbed(const std::string& _username, bool _joined);

	struct Impl;
	std::unique_ptr<Impl> impl;

	std::mutex mutex;
	std::queue<InboundChat> inboundChat;
	std::queue<ServerTask> serverTasks;

	std::string token;
	std::string channelId;
	std::string guildId;
	std::string adminRoleId;
	std::string webhookUrl;
	std::atomic<bool> initialized{ false };
	std::atomic<bool> shuttingDown{ false };
};

Discord& GlobalDiscord();

#endif
