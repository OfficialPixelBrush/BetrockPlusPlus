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

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <format>
#include <sstream>
#include <utility>
#include <vector>

struct Discord::Impl {
	std::unique_ptr<dpp::cluster> cluster;
};

Discord::Discord() : impl(std::make_unique<Impl>()) {}

Discord::~Discord() {
	Shutdown();
}

void Discord::EnqueueServerTask(ServerTask _task) {
	std::lock_guard lock(mutex);
	if (!initialized.load())
		return;
	serverTasks.push(std::move(_task));
}

void Discord::Shutdown() {
	std::unique_ptr<dpp::cluster> cluster;
	{
		std::lock_guard lock(mutex);
		initialized.store(false);
		if (impl && impl->cluster)
			cluster = std::move(impl->cluster);
	}

	if (cluster) {
		try {
			cluster->shutdown();
		} catch (const std::exception& e) {
			GlobalLogger().warn << "Discord: error during shutdown: " << e.what() << "\n";
		}
	}
}

void Discord::Init(const std::string& _token, const std::string& _channelId, const std::string& _guildId) {
	if (initialized.load())
		return;

	token = _token;
	channelId = _channelId;
	guildId = _guildId;

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

	try {
		const uint32_t intents = dpp::i_default_intents | dpp::i_message_content;
		auto cluster = std::make_unique<dpp::cluster>(token, intents);

		cluster->on_log([](const dpp::log_t& event) {
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
			dpp::slashcommand say("say", "Broadcast a message to Minecraft chat", impl->cluster->me.id);
			say.add_option(dpp::command_option(dpp::co_string, "message", "Message to broadcast", true));
			dpp::slashcommand stop("stop", "Stop the Minecraft server", impl->cluster->me.id);
			stop.add_option(dpp::command_option(dpp::co_number, "seconds", "Optional countdown in seconds", false));

			const std::vector<dpp::slashcommand> commands{ status, list, version, say, stop };

			if (!guildId.empty()) {
				impl->cluster->guild_bulk_command_create(commands, dpp::snowflake{ guildId });
				GlobalLogger().info << "Discord: registered guild slash commands\n";
			} else {
				impl->cluster->global_bulk_command_create(commands);
				GlobalLogger().info << "Discord: registered global slash commands (may take up to an hour)\n";
			}
		});

		cluster->on_message_create([this, channelSnowflake](const dpp::message_create_t& event) {
			if (!initialized.load())
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
			chat.content = event.msg.content;

			std::lock_guard lock(mutex);
			inboundChat.push(std::move(chat));
		});

		cluster->on_slashcommand([this](const dpp::slashcommand_t& event) {
			const std::string name = event.command.get_command_name();

			if (name == "status") {
				event.thinking(true);
				EnqueueServerTask([event](Server& server) mutable {
					size_t online = 0;
					for (const auto& player : server.GetPlayers()) {
						if (player && player->connState == ConnectionState::Playing)
							++online;
					}
					event.edit_original_response(dpp::message(
					    std::format("{} {} — {} player(s) online, avg tick {:.2f} ms", PROJECT_NAME,
					                PROJECT_VERSION_FULL_STRING, online, server.averageTickMs)));
				});
				return;
			}

			if (name == "list") {
				event.thinking(true);
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
					const std::string reply =
					    online == 0 ? "No players online." : std::format("{} player(s): {}", online, oss.str());
					event.edit_original_response(dpp::message(reply));
				});
				return;
			}

			if (name == "version") {
				event.reply(std::format("{} {}", PROJECT_NAME, PROJECT_VERSION_FULL_STRING));
				return;
			}

			if (name == "say") {
				std::string message;
				try {
					message = std::get<std::string>(event.get_parameter("message"));
				} catch (...) {
					event.reply(dpp::message("Missing message.").set_flags(dpp::m_ephemeral));
					return;
				}
				if (message.empty()) {
					event.reply(dpp::message("Message cannot be empty.").set_flags(dpp::m_ephemeral));
					return;
				}

				const std::string nick = event.command.member.get_nickname();
				const std::string author = !nick.empty() ? nick : event.command.usr.username;
				event.thinking(true);
				EnqueueServerTask([event, author, message](Server& server) mutable {
					BroadcastDiscordChat(server, author, message);
					event.edit_original_response(dpp::message("Sent."));
				});
				return;
			}

			if (name == "stop") {
				double seconds = 0.0;
				try {
					seconds = std::get<double>(event.get_parameter("seconds"));
				} catch (...) {
					seconds = 0.0;
				}

				event.thinking(true);
				EnqueueServerTask([event, seconds](Server& server) mutable {
					if (seconds > 0.0) {
						static constexpr float MAX_TIMEOUT =
						    UINT16_MAX / static_cast<float>(Server::TICKS_PER_SECOND);
						if (seconds > MAX_TIMEOUT) {
							event.edit_original_response(
							    dpp::message(std::format("Exceeds max timeout ({} seconds).", MAX_TIMEOUT)));
							return;
						}
						server.SendGlobalChatMessage(std::format("§eStopping in {:.1f} seconds...", seconds), false);
						server.StopTimeout(static_cast<float>(seconds));
						event.edit_original_response(
						    dpp::message(std::format("Stopping in {:.1f} seconds.", seconds)));
					} else {
						server.SendGlobalChatMessage("§eStopping...", false);
						shutdownRequested.store(true);
						event.edit_original_response(dpp::message("Stop requested."));
					}
				});
				return;
			}

			event.reply(dpp::message("Unknown command.").set_flags(dpp::m_ephemeral));
		});

		impl->cluster = std::move(cluster);
		impl->cluster->start(dpp::st_return);
		initialized.store(true);
		GlobalLogger().info << "Discord: Gateway bot started\n";
	} catch (const std::exception& e) {
		GlobalLogger().error << "Discord: failed to start bot: " << e.what() << "\n";
		initialized.store(false);
		impl->cluster.reset();
	}
}

void Discord::SendMessage(const std::string& _message) {
	dpp::cluster* cluster = nullptr;
	{
		std::lock_guard lock(mutex);
		if (!initialized.load() || !impl || !impl->cluster) {
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
		if (!initialized.load() || !impl || !impl->cluster) {
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

void Discord::SendMessageSync(const std::string& _message) {
	if (token.empty() || channelId.empty())
		return;

	// Fresh cluster: CrashCatch may run this in a forked child where the live bot threads
	// do not exist. REST-only calls avoid shared Gateway state.
	try {
		dpp::cluster bot(token, 0);
		bot.start(dpp::st_return);

		std::mutex doneMutex;
		std::condition_variable doneCv;
		bool done = false;

		const std::string deformatted = RemoveMinecraftFormatting(_message);
		bot.message_create(dpp::message(dpp::snowflake{ channelId }, deformatted),
		                   [&](const dpp::confirmation_callback_t&) {
			                   {
				                   std::lock_guard lock(doneMutex);
				                   done = true;
			                   }
			                   doneCv.notify_one();
		                   });

		{
			std::unique_lock lock(doneMutex);
			doneCv.wait_for(lock, std::chrono::seconds(10), [&] { return done; });
		}
		bot.shutdown();
	} catch (const std::exception& e) {
		GlobalLogger().warn << "Discord: sync message failed: " << e.what() << "\n";
	}
}

void Discord::SendFileSync(const std::string& _filename, const std::string& _message) {
	if (token.empty() || channelId.empty())
		return;

	try {
		dpp::cluster bot(token, 0);
		bot.start(dpp::st_return);

		std::mutex doneMutex;
		std::condition_variable doneCv;
		bool done = false;

		dpp::message msg(dpp::snowflake{ channelId }, _message);
		msg.add_file(std::filesystem::path(_filename).filename().string(), dpp::utility::read_file(_filename));
		bot.message_create(msg, [&](const dpp::confirmation_callback_t&) {
			{
				std::lock_guard lock(doneMutex);
				done = true;
			}
			doneCv.notify_one();
		});

		{
			std::unique_lock lock(doneMutex);
			doneCv.wait_for(lock, std::chrono::seconds(15), [&] { return done; });
		}
		bot.shutdown();
	} catch (const std::exception& e) {
		GlobalLogger().warn << "Discord: sync file upload failed: " << e.what() << "\n";
	}
}

void Discord::Drain(Server& _server) {
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

	// §9[…] §f  — leave room for brackets/colors even if the name is huge
	auto makePrefix = [](const std::string& name) { return "§9[" + name + "] §f"; };

	std::string prefix = makePrefix(author);
	if (prefix.size() >= MAX_LINE) {
		const size_t overhead = makePrefix("").size(); // §9[] §f
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

	for (size_t offset = 0; offset < content.size(); offset += contentBudget) {
		lines.push_back(prefix + content.substr(offset, contentBudget));
	}
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
