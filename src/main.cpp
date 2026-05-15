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

#ifdef __3DS__
static u32* soc_buffer = nullptr;
#include <malloc.h>
#endif

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    #if defined(_WIN32) || defined(_WIN64)
        GlobalLogger().info << "Running on Windows (" << BUILD_MODE << ")\n";
        SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
    #elif defined(__linux__)
        GlobalLogger().info << "Running on Linux (" << BUILD_MODE << ")\n";
    #elif defined(__APPLE__)
        GlobalLogger().info << "Running on macOS (" << BUILD_MODE << ")\n";
    #elif defined(__SWITCH__)
        GlobalLogger().info << "Running on Nintendo Switch (" << BUILD_MODE << ")\n";
    #elif defined(__3DS__)
        GlobalLogger().info << "Running on Nintendo 3DS (" << BUILD_MODE << ")\n";
    #else
        GlobalLogger().warn << "Running on an unknown/unsupported platform (" << BUILD_MODE << ")\n" << "Unexpected bugs may occur!\n";
    #endif

    #ifdef __SWITCH__
    consoleInit(NULL);
    socketInitializeDefault();
    #endif
    #ifdef __3DS__
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    hidScanInput();
    consoleClear();
    printf("BOOT 1\n");
    soc_buffer = (u32*)memalign(0x1000, 0x100000);
    if (!soc_buffer) {
        printf("Failed to allocate SOC buffer\n");
        return 1;
    }

    Result ret = socInit(soc_buffer, 0x100000);
    if (ret != 0) {
        printf("socInit failed: 0x%08X\n", (unsigned int)ret);
        return 1;
    }

    printf("SOC initialized!\n");
    #endif

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    #ifdef BUILD_SERVER
        try {
            Server serv;
            server = &serv;
            server->run();
        }
        catch (const std::exception& e) {
            printf("EXCEPTION: %s\n", e.what());
            while (aptMainLoop()) {
                gfxFlushBuffers();
                gfxSwapBuffers();
                gspWaitForVBlank();
            }
        }
    #endif
    #ifdef BUILD_CLIENT
        Client client;
        client.run();
    #endif

    #ifdef __SWITCH__
        socketExit();
    #endif

    #ifdef __3DS__
        socExit();
        gfxExit();
    #endif

    return 0;
}