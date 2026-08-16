/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "bpp_server/server.h"
#include "bpp_shared/helpers/java/java_math.h"
#include "logger.h"
#include "platforms.h"
#include "quick_arg_parser.hpp"
#include "version.h"
#include <atomic>
#include <csignal>
#include <fstream>
#include <numeric_structs.h>
#include <sstream>

#ifndef BUILD_SERVER
#include "bpp_client/client.h"
#else
#ifdef DISCORD_INTEGRATION
#include "discord.h"
#endif
#endif

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#endif

#include "bpp_utilities/utilities.h"
#include <format>

Server* server;
std::atomic<bool> shutdownRequested{ false };

#if defined(_WIN32) || defined(_WIN64)
BOOL WINAPI consoleCtrlHandler(DWORD dwCtrlType) {
	switch (dwCtrlType) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_SHUTDOWN_EVENT:
	case CTRL_LOGOFF_EVENT:
		shutdownRequested.store(true);
		// Block so the OS doesn't kill us before the main thread finishes saving
		while (shutdownRequested.load())
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		return TRUE;
	default:
		return FALSE;
	}
}
#endif

// Fall back to being a server if neither are defined
#if !defined(BUILD_SERVER) && !defined(BUILD_CLIENT)
#define BUILD_SERVER
#endif

static void SignalHandler(int /*sig*/) {
	shutdownRequested.store(true);
}

struct Args : MainArguments<Args> {
	[[maybe_unused]] inline static const std::string Version() noexcept {
		return std::string(PROJECT_FULL_VERSION_LABEL);
	}
	uint16_t port = option("port", 'p', "Port the server will run on (25565)") = 25565;
	int32_t maxPlayers = option("max_players", '\0',
	                            "Maximum number of players. Anything less than 0 removes the cap (-1)") = -1;
	bool enableWhitelist = (option("whitelist", 'w', "Enables usage of the whitelist") = false);
	int64_t seed = option("seed", 's', "Overwrites the worlds current seed") = 0;
	bool disablePortals = (option("no_portals", '\0', "Disables Portal-travel") = false);
	bool forceNetherSpawn = (option("force_nether_spawn", '\0', "Makes players spawn in the Nether") = false);
	uint32_t pregenRadius = option("pregen_radius", '\0',
	                               "Generates chunks around 0,0 until the desired radius is met") = 5;
	uint32_t chunkRenderRadius = option(
	    "chunk_render_radius", '\0',
	    "Radius within which chunks are rendered for clients. On Vanilla clients this caps out at about 16 Chunks") = 5;
	uint32_t chunkGenRadius = option("chunk_gen_radius", '\0', "Radius within which chunks are generated") = 5;
	uint32_t chunkTickRadius = option("chunk_tick_radius", '\0', "Radius within which chunks are randomly ticked") = 5;
	uint32_t entityRenderRadius = option("entity_render_radius", '\0', "Radius within which entities are shown") = 5;
	uint32_t entityTickRadius = option("entity_tick_radius", '\0', "Radius within which entities are ticked") = 5;
};

#ifdef CRASH_LOGGING
#include "CrashCatch.hpp"
void InitCrashHandler(std::string _platformString) {
	CrashCatch::Config config{ .dumpFolder = "./",
		                       // TODO: Apparently enableTextLog only affects stuff on Windows? TEST!!!
		                       .enableTextLog = false,
		                       .autoTimestamp = true,
#if defined(BUILD_SERVER)
		                       .showCrashDialog = false,
#else
		                       .showCrashDialog = true,
#endif
		                       .appVersion = std::string(PROJECT_VERSION_FULL_STRING),
		                       .buildConfig = _platformString };
	config.onCrash = [](const CrashCatch::CrashContext& _ctx) {
		auto& log = GlobalLogger();

		log.error << "========== CRASH ==========\n";
		log.error << "Signal/Code: " << _ctx.signalOrCode << "\n";

		if (!_ctx.logFilePath.empty()) {
			log.error << "Crash report: " << _ctx.logFilePath << "\n";

			std::ifstream file(_ctx.logFilePath);
			if (!file) {
				GlobalLogger().error << "Failed to open crash report file!\n";
#ifdef DISCORD_INTEGRATION
				std::ostringstream summary;
				summary << "**Server crashed!** Signal/Code: " << _ctx.signalOrCode
				        << "\n(crash report file could not be opened for upload)";
				GlobalDiscord().SendMessageSync(summary.str());
#endif
				return;
			}

			std::string line;

			while (std::getline(file, line)) {
				GlobalLogger().error << line << "\n";
			}

#ifdef DISCORD_INTEGRATION
			std::ostringstream summary;
			summary << "**Server crashed!** Signal/Code: " << _ctx.signalOrCode;
			GlobalDiscord().SendFileSync(std::string(_ctx.logFilePath), summary.str());
#endif
		}
#ifdef DISCORD_INTEGRATION
		else {
			std::ostringstream summary;
			summary << "**Server crashed!** Signal/Code: " << _ctx.signalOrCode
			        << "\n(no crash report file was produced)";
			GlobalDiscord().SendMessageSync(summary.str());
		}
#endif
	};
	CrashCatch::initialize(config);
}
#endif

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
	std::string platformString = std::format("{} ({}, {})", PLATFORM_NAME, BUILD_MODE, ARCH_NAME);
#ifdef CRASH_LOGGING
	InitCrashHandler(platformString);
#endif
	// Hook up signals
	std::signal(SIGINT, SignalHandler);
	std::signal(SIGTERM, SignalHandler);
#ifdef SIGPIPE
	// Ignore broken pipes caused by early disconnecting client
	std::signal(SIGPIPE, SIG_IGN);
#endif
	// Parse CLI Args
	Args args{ { argc, argv } };
	// Init the sine table
	MathHelper::InitSinTable();
	// We're ready to roll
	GlobalLogger().info << "Running on " << platformString << "\n";
#if defined(_WIN32) || defined(_WIN64)
	SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif

#ifdef BUILD_SERVER
	// For testing REMOVE LATER!
	// std::string path = "";
	// Utilities::convertBetrockServerLevel(/*path=*/path);
	Server serv;
	server = &serv;
	server->Run();
#endif
#ifdef BUILD_CLIENT
	Client client;
	client.Run();
#endif

	return 0;
}