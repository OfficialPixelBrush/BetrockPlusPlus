/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#include <iostream>
#include <numeric_structs.h>
#include "bpp_shared/NBT/example.h"
#include "bpp_client/client.h"
#include "bpp_test/network_test.h"
#include "networking/network_stream.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    bool nbt_tests = false;
    bool server_tests = false;
    bool client_tests = !server_tests;
    bool graphical_client_tests = true;

    #if defined(_WIN32) || defined(_WIN64)
        std::cout << "Running on Windows\n";
    #elif defined(__linux__)
        std::cout << "Running on Linux\n";
    #else
        std::cout << "Running on an unknown/unsupported platform\n";
    #endif

    // Test NBT example
    if (nbt_tests)
        NBTexample::test();
    if (server_tests)
        ServerTest();
    if (client_tests) {
        if (graphical_client_tests) {
            Client client;
            client.run();
        }
        //ClientTest();
    }

    return 0;
}