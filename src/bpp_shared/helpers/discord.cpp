/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "discord.h"

#ifdef DISCORD_INTEGRATION

#include "logger.h"
#include "server.h"
#include "version.h"

#include <dpp/dpp.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

namespace {

void ReplaceAll(std::string& _haystack, const std::string& _from, const std::string& _to) {
	if (_from.empty())
		return;
	size_t pos = 0;
	while ((pos = _haystack.find(_from, pos)) != std::string::npos) {
		_haystack.replace(pos, _from.size(), _to);
		pos += _to.size();
	}
}

std::string DisplayNameForMention(const dpp::user& _user, const dpp::guild_member& _member) {
	const std::string nick = _member.get_nickname();
	if (!nick.empty())
		return nick;
	if (!_user.global_name.empty())
		return _user.global_name;
	return _user.username;
}

// Discord stores mentions as <@id> / <@!id> / <#id> / <@&id>; Minecraft has no Discord
// mention renderer, so resolve them to readable @name / #name before broadcasting.
std::string ResolveDiscordMentions(const dpp::message& _msg) {
	std::string content = _msg.content;

	for (const auto& [user, member] : _msg.mentions) {
		const std::string idStr = std::to_string(static_cast<uint64_t>(user.id));
		const std::string label = "@" + DisplayNameForMention(user, member);
		ReplaceAll(content, "<@!" + idStr + ">", label);
		ReplaceAll(content, "<@" + idStr + ">", label);
	}

	for (const dpp::snowflake roleId : _msg.mention_roles) {
		const std::string idStr = std::to_string(static_cast<uint64_t>(roleId));
		std::string label = "@role";
		if (const dpp::role* role = dpp::find_role(roleId); role && !role->name.empty())
			label = "@" + role->name;
		ReplaceAll(content, "<@&" + idStr + ">", label);
	}

	for (const dpp::channel& channel : _msg.mention_channels) {
		const std::string idStr = std::to_string(static_cast<uint64_t>(channel.id));
		const std::string label = channel.name.empty() ? "#channel" : ("#" + channel.name);
		ReplaceAll(content, "<#" + idStr + ">", label);
	}

	return content;
}

bool MemberHasRole(const dpp::guild_member& _member, const std::string& _roleId) {
	if (_roleId.empty())
		return false;
	const dpp::snowflake want{ _roleId };
	if (want == 0)
		return false;
	for (const dpp::snowflake role : _member.get_roles()) {
		if (role == want)
			return true;
	}
	return false;
}

} // namespace

struct Discord::Impl {
	std::unique_ptr<dpp::cluster> cluster;
	// Base webhook (id + token), parsed once. Per-message identity (name/avatar_url)
	// is set on a copy of this before each execute_webhook() call.
	dpp::webhook chatWebhook;
	bool hasWebhook = false;
};

Discord::Discord() : impl(std::make_unique<Impl>()) {}

Discord::~Discord() {
	Shutdown();
}

void Discord::EnqueueServerTask(ServerTask _task) {
	std::lock_guard lock(mutex);
	if (!initialized.load() || shuttingDown.load())
		return;
	serverTasks.push(std::move(_task));
}

void Discord::Shutdown(const std::string& _finalMessage) {
	if (shuttingDown.exchange(true)) {
		std::unique_ptr<dpp::cluster> leftover;
		{
			std::lock_guard lock(mutex);
			initialized.store(false);
			if (impl && impl->cluster)
				leftover = std::move(impl->cluster);
		}
		// Best-effort; do not block the destructor path if Discord is wedged.
		if (leftover) {
			auto done = std::make_shared<std::atomic<bool>>(false);
			std::thread([cluster = std::move(leftover), done]() mutable {
				try {
					cluster->shutdown();
					cluster.reset();
				} catch (...) {
				}
				done->store(true);
			}).detach();
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
			while (!done->load() && std::chrono::steady_clock::now() < deadline)
				std::this_thread::sleep_for(std::chrono::milliseconds(25));
		}
		return;
	}

	std::unique_ptr<dpp::cluster> cluster;
	{
		std::lock_guard lock(mutex);
		initialized.store(false);
		while (!inboundChat.empty())
			inboundChat.pop();
		while (!serverTasks.empty())
			serverTasks.pop();
		if (impl && impl->cluster)
			cluster = std::move(impl->cluster);
	}

	if (!cluster)
		return;

	GlobalLogger().info << "Discord: shutting down...\n";

	const std::string goodbye = _finalMessage.empty() ? std::string{} : RemoveMinecraftFormatting(_finalMessage);

	// Ownership moves into a worker so a stuck SSL/event-loop join cannot freeze Ctrl+C.
	// DPP's cluster::shutdown() joins the engine thread; if that thread is blocked inside
	// OpenSSL (common when Discord HTTP is unhealthy), join never returns.
	auto done = std::make_shared<std::atomic<bool>>(false);
	std::thread worker([cluster = std::move(cluster), goodbye, channel = channelId, done]() mutable {
		try {
			if (!goodbye.empty() && !channel.empty()) {
				// Fire-and-forget goodbye — never wait on Discord here.
				cluster->message_create(dpp::message(dpp::snowflake{ channel }, goodbye),
				                        [](const dpp::confirmation_callback_t&) {});
				std::this_thread::sleep_for(std::chrono::milliseconds(150));
			}
			cluster->shutdown();
			cluster.reset();
		} catch (const std::exception& e) {
			GlobalLogger().warn << "Discord: error during shutdown: " << e.what() << "\n";
		} catch (...) {
		}
		done->store(true);
	});

	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	while (!done->load() && std::chrono::steady_clock::now() < deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

	if (done->load()) {
		worker.join();
		GlobalLogger().info << "Discord: shut down\n";
	} else {
		GlobalLogger().warn << "Discord: shutdown timed out; abandoning Gateway threads so the "
		                       "server can exit\n";
		worker.detach();
	}
}

void Discord::Init(const std::string& _token, const std::string& _channelId, const std::string& _guildId,
                   const std::string& _adminRoleId, const std::string& _webhookUrl) {
	if (initialized.load() || shuttingDown.load())
		return;

	token = _token;
	channelId = _channelId;
	guildId = _guildId;
	adminRoleId = _adminRoleId;
	webhookUrl = _webhookUrl;

	if (token.empty() || channelId.empty()) {
		GlobalLogger().warn << "Discord integration is enabled but discord-token or discord-channel-id "
		                       "is missing/empty in server.properties; Discord bot will not start.\n";
		return;
	}

	const dpp::snowflake channelSnowflake{ channelId };
	if (channelSnowflake == 0) {
		GlobalLogger().error << "Discord: discord-channel-id is not a valid snowflake\n";
		return;
	}

	if (!guildId.empty()) {
		const dpp::snowflake guildSnowflake{ guildId };
		if (guildSnowflake == 0) {
			GlobalLogger().warn << "Discord: discord-guild-id is invalid; registering slash commands globally "
			                       "(can take up to an hour to appear).\n";
			guildId.clear();
		}
	}

	if (adminRoleId.empty()) {
		GlobalLogger().warn << "Discord: discord-admin-role-id is empty; privileged slash commands "
		                       "(/stop) will be denied until it is set.\n";
	} else {
		const dpp::snowflake adminRoleSnowflake{ adminRoleId };
		if (adminRoleSnowflake == 0) {
			GlobalLogger().warn << "Discord: discord-admin-role-id is invalid; privileged slash commands "
			                       "(/stop) will be denied.\n";
			adminRoleId.clear();
		}
	}

	if (!webhookUrl.empty()) {
		try {
			impl->chatWebhook = dpp::webhook(webhookUrl);
			impl->hasWebhook = true;
		} catch (const std::exception& e) {
			GlobalLogger().warn << "Discord: discord-webhook-url is invalid (" << e.what()
			                    << "); player chat will be relayed as the bot instead.\n";
		}
	}

	try {
		const uint32_t intents = dpp::i_default_intents | dpp::i_message_content;
		auto cluster = std::make_unique<dpp::cluster>(token, intents);

		cluster->on_log([](const dpp::log_t& event) {
			// 10062 = Unknown interaction (acked too late / raced). Noise once we reply properly.
			if (event.message.find("10062") != std::string::npos)
				return;
			if (event.severity >= dpp::ll_error) {
				GlobalLogger().error << "Discord: " << event.message << "\n";
			} else if (event.severity == dpp::ll_warning) {
				GlobalLogger().warn << "Discord: " << event.message << "\n";
			}
		});

		cluster->on_ready([this](const dpp::ready_t&) {
			if (!dpp::run_once<struct register_bot_commands>())
				return;
			if (!impl || !impl->cluster)
				return;

			dpp::slashcommand status("status", "Show Minecraft server status", impl->cluster->me.id);
			dpp::slashcommand list("list", "List online Minecraft players", impl->cluster->me.id);
			dpp::slashcommand version("version", "Show Betrock++ version", impl->cluster->me.id);
			dpp::slashcommand stop("stop", "Stop the Minecraft server", impl->cluster->me.id);
			stop.add_option(dpp::command_option(dpp::co_number, "seconds", "Optional countdown in seconds", false));

			// Bulk create replaces the guild/global command set — omitting /say removes it from Discord.
			const std::vector<dpp::slashcommand> commands{ status, list, version, stop };

			if (!guildId.empty()) {
				impl->cluster->guild_bulk_command_create(commands, dpp::snowflake{ guildId });
				GlobalLogger().info << "Discord: registered guild slash commands\n";
			} else {
				impl->cluster->global_bulk_command_create(commands);
				GlobalLogger().info << "Discord: registered global slash commands (may take up to an hour)\n";
			}
		});

		cluster->on_message_create([this, channelSnowflake](const dpp::message_create_t& event) {
			if (!initialized.load() || shuttingDown.load())
				return;
			if (event.msg.channel_id != channelSnowflake)
				return;
			if (event.msg.author.is_bot())
				return;
			if (event.msg.content.empty())
				return;

			InboundChat chat;
			const std::string nick = event.msg.member.get_nickname();
			chat.author = !nick.empty() ? nick : event.msg.author.username;
			chat.content = ResolveDiscordMentions(event.msg);

			std::lock_guard lock(mutex);
			inboundChat.push(std::move(chat));
		});

		cluster->on_slashcommand([this](const dpp::slashcommand_t& event) {
			if (!initialized.load() || shuttingDown.load())
				return;

			const std::string name = event.command.get_command_name();

			// Reply/ack immediately on the DPP thread. Never edit before thinking/reply
			// completes — that races and yields Discord error 10062 (Unknown interaction),
			// and during /stop it coincided with cluster teardown (OpenSSL segfault).

			if (name == "status") {
				event.thinking(true, [this, event](const dpp::confirmation_callback_t& result) {
					if (result.is_error())
						return;
					EnqueueServerTask([event](Server& server) mutable {
						size_t online = 0;
						for (const auto& player : server.GetPlayers()) {
							if (player && player->connState == ConnectionState::Playing)
								++online;
						}
						event.edit_original_response(
						    dpp::message(std::format("{} {} — {} player(s) online, avg tick {:.2f} ms", PROJECT_NAME,
						                             PROJECT_VERSION_FULL_STRING, online, server.averageTickMs)));
					});
				});
				return;
			}

			if (name == "list") {
				event.thinking(true, [this, event](const dpp::confirmation_callback_t& result) {
					if (result.is_error())
						return;
					EnqueueServerTask([event](Server& server) mutable {
						std::ostringstream oss;
						size_t online = 0;
						for (const auto& player : server.GetPlayers()) {
							if (!player || player->connState != ConnectionState::Playing)
								continue;
							if (online > 0)
								oss << ", ";
							oss << player->username;
							++online;
						}
						const std::string reply = online == 0 ? "No players online."
						                                      : std::format("{} player(s): {}", online, oss.str());
						event.edit_original_response(dpp::message(reply));
					});
				});
				return;
			}

			if (name == "version") {
				event.reply(std::format("{} {}", PROJECT_NAME, PROJECT_VERSION_FULL_STRING));
				return;
			}

			if (name == "stop") {
				if (!MemberHasRole(event.command.member, adminRoleId)) {
					event.reply(
					    dpp::message("You need the configured admin role to run /stop.").set_flags(dpp::m_ephemeral));
					return;
				}

				double seconds = 0.0;
				try {
					seconds = std::get<double>(event.get_parameter("seconds"));
				} catch (...) {
					seconds = 0.0;
				}

				if (seconds > 0.0) {
					static constexpr float MAX_TIMEOUT = UINT16_MAX / static_cast<float>(Server::TICKS_PER_SECOND);
					if (seconds > MAX_TIMEOUT) {
						event.reply(std::format("Exceeds max timeout ({} seconds).", MAX_TIMEOUT));
						return;
					}
					event.reply(std::format("Stopping in {:.1f} seconds.", seconds));
					EnqueueServerTask([seconds](Server& server) {
						server.SendGlobalChatMessage(std::format("§eStopping in {:.1f} seconds...", seconds), false);
						server.StopTimeout(static_cast<float>(seconds));
					});
				} else {
					// Immediate reply BEFORE requesting shutdown so the interaction is
					// finished while the cluster is still healthy.
					event.reply("Stop requested.");
					EnqueueServerTask([](Server& server) {
						server.SendGlobalChatMessage("§eStopping...", false);
						shutdownRequested.store(true);
					});
				}
				return;
			}

			event.reply(dpp::message("Unknown command.").set_flags(dpp::m_ephemeral));
		});

		impl->cluster = std::move(cluster);
		impl->cluster->start(dpp::st_return);
		initialized.store(true);
		GlobalLogger().info << "Discord: Gateway bot started\n";

		// A webhook URL can silently point at the wrong channel (e.g. copied from a
		// different channel than discord-channel-id) or be stale (deleted/regenerated).
		// Either failure mode otherwise looks identical from here: chat just never
		// shows up with no error visible outside the log. Check once at startup so
		// that's obvious immediately instead of only surfacing per-message warnings.
		if (impl->hasWebhook) {
			const dpp::snowflake webhookId = impl->chatWebhook.id;
			const std::string webhookToken = impl->chatWebhook.token;
			impl->cluster->get_webhook_with_token(
			    webhookId, webhookToken, [channelSnowflake](const dpp::confirmation_callback_t& result) {
				    if (result.is_error()) {
					    GlobalLogger().error << "Discord: discord-webhook-url could not be verified ("
					                         << result.get_error().message
					                         << "); it is likely deleted/regenerated. Player chat will fail to relay "
					                            "until it's replaced with a current webhook URL.\n";
					    return;
				    }
				    const dpp::webhook wh = result.get<dpp::webhook>();
				    if (wh.channel_id != channelSnowflake) {
					    GlobalLogger().warn << "Discord: discord-webhook-url posts to channel " << wh.channel_id
					                        << ", but discord-channel-id is " << channelSnowflake
					                        << ". Player chat will relay to a different channel than /status, "
					                           "/list, and join/leave messages; create the webhook on the same "
					                           "channel or update discord-channel-id.\n";
				    } else {
					    GlobalLogger().info << "Discord: webhook verified, chat will relay to channel " << wh.channel_id
					                        << "\n";
				    }
			    });
		}
	} catch (const std::exception& e) {
		GlobalLogger().error << "Discord: failed to start bot: " << e.what() << "\n";
		initialized.store(false);
		impl->cluster.reset();
		impl->hasWebhook = false;
	}
}

void Discord::SendMessage(const std::string& _message) {
	dpp::cluster* cluster = nullptr;
	{
		std::lock_guard lock(mutex);
		if (!initialized.load() || shuttingDown.load() || !impl || !impl->cluster) {
			GlobalLogger().warn << "Discord: message dropped, integration not initialized\n";
			return;
		}
		cluster = impl->cluster.get();
	}

	const std::string deformatted = RemoveMinecraftFormatting(_message);
	dpp::message msg(dpp::snowflake{ channelId }, deformatted);
	cluster->message_create(msg, [](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			GlobalLogger().warn << "Discord: failed to send message: " << result.get_error().message << "\n";
		}
	});
}

void Discord::SendFile(const std::string& _filename, const std::string& _message) {
	dpp::cluster* cluster = nullptr;
	{
		std::lock_guard lock(mutex);
		if (!initialized.load() || shuttingDown.load() || !impl || !impl->cluster) {
			GlobalLogger().warn << "Discord: file upload dropped, integration not initialized\n";
			return;
		}
		cluster = impl->cluster.get();
	}

	try {
		dpp::message msg(dpp::snowflake{ channelId }, _message);
		msg.add_file(std::filesystem::path(_filename).filename().string(), dpp::utility::read_file(_filename));
		cluster->message_create(msg, [filename = _filename](const dpp::confirmation_callback_t& result) {
			if (result.is_error()) {
				GlobalLogger().warn << "Discord: failed to send file '" << filename
				                    << "': " << result.get_error().message << "\n";
			}
		});
	} catch (const std::exception& e) {
		GlobalLogger().warn << "Discord: failed to send file '" << _filename << "': " << e.what() << "\n";
	}
}

std::string Discord::BuildSkinAvatarUrl(const std::string& _username, int _size) {
	// mc-heads.net resolves the username to a skin server-side (Java accounts and most
	// offline/cracked UUIDs alike) and returns just the face crop as a PNG. Discord fetches
	// this URL itself when it renders the message/embed, so the bot never has to download
	// or decode a skin texture — no image library or extra HTTP round trip needed here.
	std::string safe;
	safe.reserve(_username.size());
	for (const char c : _username) {
		// Minecraft usernames are already restricted to [A-Za-z0-9_]{1,16}; this is just a
		// defensive filter so nothing unexpected ends up in a URL path segment.
		if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
			safe += c;
	}
	if (safe.empty())
		safe = "MHF_Steve"; // generic fallback face if the username was unusable
	return "https://mc-heads.net/avatar/" + safe + "/" + std::to_string(_size);
}

std::string Discord::SanitizeWebhookUsername(const std::string& _username) {
	std::string name = RemoveMinecraftFormatting(_username);

	// Discord rejects webhook usernames containing "discord" or "clyde" (case-insensitive).
	// Mask rather than reject outright so a player named e.g. "Discordian" can still chat.
	auto maskSubstring = [&name](const std::string& _needle) {
		std::string lower = name;
		std::transform(lower.begin(), lower.end(), lower.begin(),
		               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		size_t pos = 0;
		while ((pos = lower.find(_needle, pos)) != std::string::npos) {
			for (size_t i = 0; i < _needle.size(); ++i) {
				name[pos + i] = '_';
				lower[pos + i] = '_';
			}
			pos += _needle.size();
		}
	};
	maskSubstring("discord");
	maskSubstring("clyde");

	if (name.size() > 80)
		name = name.substr(0, 80);
	if (name.empty())
		name = "Player";
	return name;
}

void Discord::SendPlayerChatMessage(const std::string& _username, const std::string& _message) {
	dpp::cluster* cluster = nullptr;
	bool useWebhook = false;
	dpp::webhook wh;
	{
		std::lock_guard lock(mutex);
		if (!initialized.load() || shuttingDown.load() || !impl || !impl->cluster) {
			GlobalLogger().warn << "Discord: chat message dropped, integration not initialized\n";
			return;
		}
		cluster = impl->cluster.get();
		useWebhook = impl->hasWebhook;
		if (useWebhook)
			wh = impl->chatWebhook;
	}

	const std::string content = RemoveMinecraftFormatting(_message);
	if (content.empty())
		return;

	if (useWebhook) {
		wh.name = SanitizeWebhookUsername(_username);
		wh.avatar_url = BuildSkinAvatarUrl(_username, 128);
		cluster->execute_webhook(wh, dpp::message(content), false, 0, "",
		                         [](const dpp::confirmation_callback_t& result) {
			                         if (result.is_error()) {
				                         GlobalLogger().error << "Discord: failed to relay chat via webhook: "
				                                              << result.get_error().message << "\n";
			                         }
		                         });
		return;
	}

	// No webhook configured (or it failed to parse at startup): fall back to a plain
	// message under the bot's own identity, same as before this feature existed.
	dpp::message msg(dpp::snowflake{ channelId }, std::format("[{}] {}", RemoveMinecraftFormatting(_username), content));
	cluster->message_create(msg, [](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			GlobalLogger().warn << "Discord: failed to send chat message: " << result.get_error().message << "\n";
		}
	});
}

constexpr uint32_t Discord::GetColorCode(const EmbedColor _color) const {
	switch (_color) {
	case EmbedColor::Red:
		return 0xFF5555;
	case EmbedColor::Yellow:
		return 0xFFFF55;
	case EmbedColor::Green:
		return 0x55FF55;
	case EmbedColor::Blue:
		return 0x5555FF;
	default:
		return 0x555555;
	}
}

void Discord::SendServerNotice(const std::string& _message, const EmbedColor _color) {
	dpp::cluster* cluster = nullptr;
	{
		std::lock_guard lock(mutex);
		if (!initialized.load() || shuttingDown.load() || !impl || !impl->cluster) {
			GlobalLogger().warn << "Discord: join/leave message dropped, integration not initialized\n";
			return;
		}
		cluster = impl->cluster.get();
	}

	dpp::embed embed;
	embed.set_color(GetColorCode(_color)).set_description(_message);

	dpp::message msg(dpp::snowflake{ channelId }, embed);
	cluster->message_create(msg, [](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			GlobalLogger().warn << "Discord: failed to send notice embed: " << result.get_error().message << "\n";
		}
	});
}

void Discord::SendPlayerEventEmbed(const std::string& _username, bool _joined) {
	dpp::cluster* cluster = nullptr;
	{
		std::lock_guard lock(mutex);
		if (!initialized.load() || shuttingDown.load() || !impl || !impl->cluster) {
			GlobalLogger().warn << "Discord: join/leave message dropped, integration not initialized\n";
			return;
		}
		cluster = impl->cluster.get();
	}

	const std::string username = RemoveMinecraftFormatting(_username);
	const std::string avatarUrl = BuildSkinAvatarUrl(username, 256);

	dpp::embed embed;
	embed.set_color(_joined ? 0x55FF55 : 0xFF5555)
	    .set_author(username, "", avatarUrl)
	    .set_description(std::format("**{}** {} the game", username, _joined ? "joined" : "left"));
	//.set_thumbnail(avatarUrl);

	dpp::message msg(dpp::snowflake{ channelId }, embed);
	cluster->message_create(msg, [](const dpp::confirmation_callback_t& result) {
		if (result.is_error()) {
			GlobalLogger().warn << "Discord: failed to send join/leave embed: " << result.get_error().message << "\n";
		}
	});
}

void Discord::SendPlayerJoinMessage(const std::string& _username) {
	SendPlayerEventEmbed(_username, true);
}

void Discord::SendPlayerLeaveMessage(const std::string& _username) {
	SendPlayerEventEmbed(_username, false);
}

void Discord::SendMessageSync(const std::string& _message) {
	(void)_message;
	// Post-fork CrashCatch callbacks must not touch OpenSSL/DPP: other threads'
	// locks are not inherited, which is exactly the segfault class in the crash log.
	GlobalLogger().warn << "Discord: skipping crash upload (OpenSSL/DPP is not fork-safe); see crash file on disk\n";
}

void Discord::SendFileSync(const std::string& _filename, const std::string& _message) {
	(void)_filename;
	(void)_message;
	GlobalLogger().warn << "Discord: skipping crash file upload (OpenSSL/DPP is not fork-safe); see crash file on disk\n";
}

void Discord::Drain(Server& _server) {
	if (shuttingDown.load())
		return;

	std::queue<InboundChat> chats;
	std::queue<ServerTask> tasks;
	{
		std::lock_guard lock(mutex);
		chats.swap(inboundChat);
		tasks.swap(serverTasks);
	}

	while (!chats.empty()) {
		InboundChat chat = std::move(chats.front());
		chats.pop();
		BroadcastDiscordChat(_server, chat.author, chat.content);
	}

	while (!tasks.empty()) {
		ServerTask task = std::move(tasks.front());
		tasks.pop();
		task(_server);
	}
}

std::string Discord::RemoveMinecraftFormatting(const std::string& _input) {
	std::string result;
	result.reserve(_input.size());

	for (size_t i = 0; i < _input.size(); ++i) {
		// UTF-8 encoding of § is C2 A7
		if (static_cast<unsigned char>(_input[i]) == 0xC2 && i + 1 < _input.size() &&
		    static_cast<unsigned char>(_input[i + 1]) == 0xA7) {
			i += 1;
			if (i + 1 < _input.size())
				i += 1;
			continue;
		}
		result += _input[i];
	}

	return result;
}

std::vector<std::string> Discord::FormatDiscordChatLines(const std::string& _author, const std::string& _content) {
	static constexpr size_t MAX_LINE = 119;

	std::string author = RemoveMinecraftFormatting(_author);
	std::string content = RemoveMinecraftFormatting(_content);

	auto makePrefix = [](const std::string& name) {
		return "§9[" + name + "] §f";
	};

	std::string prefix = makePrefix(author);
	if (prefix.size() >= MAX_LINE) {
		const size_t overhead = makePrefix("").size();
		const size_t maxName = overhead < MAX_LINE ? MAX_LINE - overhead : 0;
		if (author.size() > maxName)
			author = author.substr(0, maxName);
		prefix = makePrefix(author);
	}

	const size_t contentBudget = MAX_LINE > prefix.size() ? MAX_LINE - prefix.size() : 0;
	std::vector<std::string> lines;

	if (contentBudget == 0) {
		lines.push_back(prefix.substr(0, MAX_LINE));
		return lines;
	}

	if (content.empty()) {
		lines.push_back(prefix);
		return lines;
	}

	for (size_t offset = 0; offset < content.size(); offset += contentBudget)
		lines.push_back(prefix + content.substr(offset, contentBudget));

	return lines;
}

void Discord::BroadcastDiscordChat(Server& _server, const std::string& _author, const std::string& _content) {
	for (const std::string& line : FormatDiscordChatLines(_author, _content))
		_server.SendGlobalChatMessage(line, false);
}

Discord& GlobalDiscord() {
	static Discord discord;
	return discord;
}

#endif
