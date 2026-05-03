/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/
#include <iostream>
#include <thread>
#include <numeric_structs.h>
#include "bpp_shared/NBT/example.h"
#include "bpp_client/client.h"
#include "bpp_server/server.h"
#include "networking/network_stream.h"

Server server;

void signalHandler(int sig) {
    std::cout << "Interrupt handle " << sig << std::endl;
    server.stop();
    
    // Optionally exit the program after handling
    exit(sig);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    bool nbt_tests = false;
    bool server_tests = true;
    bool client_tests = false;
    bool graphical_client = false;

    #if defined(_WIN32) || defined(_WIN64)
        std::cout << "Running on Windows\n";
    #elif defined(__linux__)
        std::cout << "Running on Linux\n";
    #else
        std::cout << "Running on an unknown/unsupported platform\nUnexpected bugs may occur!\n";
    #endif

    if (nbt_tests)
        NBTexample::test();

    if (server_tests) {
        server.run();
    }

    if (client_tests) {
        if (graphical_client) {
            Client client;
            client.run();
        }
    }

    return 0;
}