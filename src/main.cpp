/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "CrashCatch.hpp"
#include "bpp_server/server.h"
#include "bpp_shared/helpers/java/java_math.h"
#include "logger.h"
#include "platforms.h"
#include "quick_arg_parser.hpp"
#include "version.h"
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <numeric_structs.h>
#include <sstream>
#include <vector>

#ifdef DISCORD_INTEGRATION
#include "discord.h"
#endif

#ifndef BUILD_SERVER
#include "bpp_client/client.h"
#endif

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#endif

#include "bpp_utilities/utilities.h"
#include <format>

#if defined(__linux__) || defined(__APPLE__)
#include <sys/wait.h>
#include <unistd.h>
#endif

Server* server;
std::atomic<bool> shutdownRequested{ false };

#ifdef DISCORD_INTEGRATION
// Populated by Server from server.properties once config is loaded (see server.cpp).
// Kept as plain globals (rather than reading GlobalDiscord()'s private members) so the
// crash handler can reach them without depending on Discord's async worker thread/queue.
std::string g_discordToken;
std::string g_discordChannelId;
#endif

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

#ifdef DISCORD_INTEGRATION
// Hidden CLI mode used internally by the crash handler to upload a crash report to
// Discord. Invoked as: <exe> --crash-upload <channelId> <logPath> <summary>
// with the bot token passed via the BPP_DISCORD_TOKEN environment variable.
//
// Why this exists: CrashCatch's Linux signal handler responds to a crash by fork()ing
// (see linuxSignalHandler in CrashCatch.hpp) and running the rest of the crash-reporting
// work - including this app's onCrash callback - in that child, WITHOUT exec()ing.
// The parent process is multithreaded (GlobalDiscord() alone runs its own worker
// thread), and fork() only duplicates the thread that called it. Any lock libcurl,
// OpenSSL, or the DNS resolver held on another thread at the moment of the crash is
// inherited by the child in a permanently "locked" state, since the thread that owns
// it doesn't exist there to release it. If the crash handler then touches curl's
// global state directly in that child (curl_global_init/cleanup, or even the first
// curl_easy_init), it can deadlock or crash - which is exactly what the attached crash
// report shows: a segfault deep inside libcurl.so, reached from linuxSignalHandler.
//
// The fix is to never run curl in that child at all. Instead we exec() a brand new
// copy of this binary in this hidden mode. exec() fully replaces the process image,
// discarding every bit of inherited (possibly corrupted) thread and lock state, so the
// resulting process is single-threaded and pristine - safe for curl_global_init().
static constexpr const char* kCrashUploadFlag = "--crash-upload";

static int RunCrashUploadHelper(int argc, char** argv) {
	// argv: [0]=self [1]=--crash-upload [2]=channelId [3]=logPath [4]=summary
	if (argc < 5)
		return 1;

	const char* tokenEnv = std::getenv("BPP_DISCORD_TOKEN");
	std::string token = tokenEnv ? tokenEnv : "";
	std::string channelId = argv[2];
	std::string logPath = argv[3];
	std::string summary = argv[4];

	if (!logPath.empty()) {
		Discord::SendFileSyncStatic(token, channelId, logPath, summary);
	} else {
		Discord::SendMessageSyncStatic(token, channelId, summary);
	}

	return 0;
}

// Spawns a fresh copy of this binary in the hidden upload mode above and waits for it
// to finish. Safe to call from CrashCatch's forked-but-not-exec'd child, because the
// actual curl work happens in the exec'd grandchild, not here.
static void SpawnCrashUploadHelper(const std::string& _token, const std::string& _channelId,
                                    const std::string& _logPath, const std::string& _summary) {
#if defined(__linux__) || defined(__APPLE__)
	pid_t pid = fork();
	if (pid == 0) {
		// Grandchild: about to exec, so it's fine that we're still inside the
		// crash handler's forked-but-not-exec'd child here.
		setenv("BPP_DISCORD_TOKEN", _token.c_str(), 1);

		std::string exePath = CrashCatch::getExecutablePath();
		std::vector<char*> execArgs = { exePath.data(),
			                             const_cast<char*>(kCrashUploadFlag),
			                             const_cast<char*>(_channelId.c_str()),
			                             const_cast<char*>(_logPath.c_str()),
			                             const_cast<char*>(_summary.c_str()),
			                             nullptr };
		execv(exePath.c_str(), execArgs.data());
		_exit(127); // exec failed
	} else if (pid > 0) {
		waitpid(pid, nullptr, 0);
	}
	// pid < 0 (fork failed): silently give up on the upload, nothing else we can do here.
#endif
}
#endif

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

		// NOTE: on Linux this callback runs inside CrashCatch's forked child process
		// (see linuxSignalHandler in CrashCatch.hpp), not the original crashed process
		// or thread, and that child was never exec()'d. GlobalDiscord()'s async worker
		// thread does not exist here, but the *memory* of libcurl/OpenSSL's global
		// state as it stood in the parent - possibly mid-use, possibly holding a lock
		// on behalf of that now-nonexistent thread - does. Calling into curl directly
		// here (even via the *Sync variants) is unsafe and can crash inside libcurl.
		// Always go through SpawnCrashUploadHelper, which does the actual curl work in
		// a freshly exec'd, single-threaded process instead.
		if (!_ctx.logFilePath.empty()) {
			log.error << "Crash report: " << _ctx.logFilePath << "\n";

			std::ifstream file(_ctx.logFilePath);
			if (!file) {
				GlobalLogger().error << "Failed to open crash report file!\n";
#ifdef DISCORD_INTEGRATION
				std::ostringstream summary;
				summary << "**Server crashed!** Signal/Code: " << _ctx.signalOrCode
				        << "\n(crash report file could not be opened for upload)";
				SpawnCrashUploadHelper(g_discordToken, g_discordChannelId, "", summary.str());
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
			SpawnCrashUploadHelper(g_discordToken, g_discordChannelId, _ctx.logFilePath, summary.str());
#endif
		}
#ifdef DISCORD_INTEGRATION
		else {
			std::ostringstream summary;
			summary << "**Server crashed!** Signal/Code: " << _ctx.signalOrCode
			        << "\n(no crash report file was produced)";
			SpawnCrashUploadHelper(g_discordToken, g_discordChannelId, "", summary.str());
		}
#endif
	};
	CrashCatch::initialize(config);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
#ifdef DISCORD_INTEGRATION
	// Hidden mode, only ever reached via execv() from SpawnCrashUploadHelper. Handled
	// first, before anything else touches a thread, socket, or global, so this process
	// stays exactly what it needs to be: single-threaded and pristine, which is what
	// makes it safe to call into curl here at all. See RunCrashUploadHelper above.
	if (argc >= 2 && std::string(argv[1]) == kCrashUploadFlag) {
		return RunCrashUploadHelper(argc, argv);
	}
#endif

	std::string platformString = std::format("{} ({}, {})", PLATFORM_NAME, BUILD_MODE, ARCH_NAME);
	InitCrashHandler(platformString);
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