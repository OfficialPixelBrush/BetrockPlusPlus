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

Server server;

static void shutdown() {
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
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

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    bool nbt_tests = false;
    bool server_tests = true;
    bool client_tests = false;
    bool graphical_client = false;

    #if defined(_WIN32) || defined(_WIN64)
        std::cout << "Running on Windows\n";
        SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
    #elif defined(__linux__)
        std::cout << "Running on Linux\n";
    #elif defined(__APPLE__)
        std::cout << "Running on macOS\n";
    #else
        std::cout << "Running on an unknown/unsupported platform\nUnexpected bugs may occur!\n";
    #endif

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    if (nbt_tests)
        NBTexample::test();

    if (server_tests)
        server.run();

    if (client_tests && graphical_client) {
        Client client;
        client.run();
    }

    return 0;
}