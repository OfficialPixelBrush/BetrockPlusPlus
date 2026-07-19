/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "bpp_shared/helpers/java/java_math.h"
#include "platforms.h"
#include "version.h"
#include <atomic>
#include <csignal>
#include <numeric_structs.h>
#ifndef BUILD_SERVER
#include "bpp_client/client.h"
#endif
#include "bpp_server/server.h"

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#endif

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

#include "quick_arg_parser.hpp"

struct Args : MainArguments<Args> {
	[[maybe_unused]] inline static const std::string version() noexcept {
		return std::string(PROJECT_FULL_VERSION_LABEL);
	}
	uint16_t m_port = option("port", 'p', "Port the server will run on (25565)") = 25565;
	int32_t m_max_players = option("max_players", '\0',
	                             "Maximum number of players. Anything less than 0 removes the cap (-1)") = -1;
	bool m_enable_whitelist = (option("whitelist", 'w', "Enables usage of the whitelist") = false);
	int64_t m_seed = option("seed", 's', "Overwrites the worlds current seed") = 0;
	bool m_disable_portals = (option("no_portals", '\0', "Disables Portal-travel") = false);
	bool m_force_nether_spawn = (option("force_nether_spawn", '\0', "Makes players spawn in the Nether") = false);
	uint32_t m_pregen_radius = option("pregen_radius", '\0',
	                                "Generates chunks around 0,0 until the desired radius is met") = 5;
	uint32_t m_chunk_render_radius = option(
	    "chunk_render_radius", '\0',
	    "Radius within which chunks are rendered for clients. On Vanilla clients this caps out at about 16 Chunks") = 5;
	uint32_t m_chunk_gen_radius = option("chunk_gen_radius", '\0', "Radius within which chunks are generated") = 5;
	uint32_t m_chunk_tick_radius = option("chunk_tick_radius", '\0', "Radius within which chunks are randomly ticked") = 5;
	uint32_t m_entity_render_radius = option("entity_render_radius", '\0', "Radius within which entities are shown") = 5;
	uint32_t m_entity_tick_radius = option("entity_tick_radius", '\0', "Radius within which entities are ticked") = 5;
};

static void signalHandler(int /*sig*/) {
	shutdownRequested.store(true);
}

// Fall back to being a server if neither are defined
#if !defined(BUILD_SERVER) && !defined(BUILD_CLIENT)
#define BUILD_SERVER
#endif

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
	// Hook up signals
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);
#ifdef SIGPIPE
	// Ignore broken pipes caused by early disconnecting client
	std::signal(SIGPIPE, SIG_IGN);
#endif
	// Parse CLI Args
	Args args{ { argc, argv } };
	// Init the sine table
	MathHelper::InitSinTable();
	// We're ready to roll
	GlobalLogger().m_info << "Running on " << PLATFORM_NAME << " (" << BUILD_MODE << ", " << ARCH_NAME << ")\n";
#if defined(_WIN32) || defined(_WIN64)
	SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif

#ifdef BUILD_SERVER
	Server serv;
	server = &serv;
	server->run();
#endif
#ifdef BUILD_CLIENT
	Client client;
	client.run();
#endif

	return 0;
}