/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include <iostream>
#include <thread>
#include <csignal>
#include <numeric_structs.h>
#include "bpp_shared/NBT/example.h"
#include "bpp_client/client.h"
#include "bpp_server/server.h"
#include "networking/network_stream.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

Server* server;

static void shutdown() {
    if (server)
        server->stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    //free(server)
}

#if defined(_WIN32) || defined(_WIN64)
BOOL WINAPI consoleCtrlHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
    case CTRL_LOGOFF_EVENT:
        shutdown();
        return TRUE;
    default:
        return FALSE;
    }
}
#endif

static void signalHandler(int sig) {
    shutdown();
    exit(sig);
}

#ifdef NDEBUG
#define BUILD_MODE "Release"
#else
#define BUILD_MODE "Debug"
#endif

// Fall back to being a server if neither are defined
#if !defined(BUILD_SERVER) && !defined(BUILD_CLIENT)
    #define BUILD_SERVER
#endif

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    #if defined(_WIN32) || defined(_WIN64)
        GlobalLogger().info << "Running on Windows (" << BUILD_MODE << ")\n";
        SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
    #elif defined(__linux__)
        GlobalLogger().info << "Running on Linux (" << BUILD_MODE << ")\n";
    #elif defined(__APPLE__)
        GlobalLogger().info << "Running on macOS (" << BUILD_MODE << ")\n";
    #else
        GlobalLogger().warn << "Running on an unknown/unsupported platform (" << BUILD_MODE << ")\n" << "Unexpected bugs may occur!\n";
    #endif

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

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