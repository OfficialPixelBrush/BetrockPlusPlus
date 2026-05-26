/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include <atomic>
#include <csignal>
#include <numeric_structs.h>
#include "bpp_shared/config/cli_args.h"
#include "bpp_shared/helpers/java/java_math.h"
#include "platforms.h"
#ifndef BUILD_SERVER
#include "bpp_client/client.h"
#endif
#include "bpp_server/server.h"

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
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

static void signalHandler(int /*sig*/) {
    shutdownRequested.store(true);
}

// Fall back to being a server if neither are defined
#if !defined(BUILD_SERVER) && !defined(BUILD_CLIENT)
#define BUILD_SERVER
#endif

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    GlobalLogger().info
        << "Running on "
        << PLATFORM_NAME
        << " ("
        << BUILD_MODE
        << ", "
        << ARCH_NAME
        << ")\n";
    #if defined(_WIN32) || defined(_WIN64)
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
    #endif

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    CommandLineArguments cliargs = ParseArgs(argc, argv);

    MathHelper::InitSinTable();
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