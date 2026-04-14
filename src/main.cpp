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
#include "bpp_shared/networking/network_stream.h"
#include "bpp_shared/networking/packets.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    #if defined(_WIN32) || defined(_WIN64)
        std::cout << "Running on Windows\n";
    #elif defined(__linux__)
        std::cout << "Running on Linux\n";
    #else
        std::cout << "Running on an unknown/unsupported platform\n";
    #endif
        
    std::cout << "--Running test suite...--\n";
    Int3 int3{2,156,16};
    std::cout << int3 << std::endl;
    std::cout << int3.x << std::endl;
    std::cout << int3.data[2] << std::endl;
    int3 = int3*5;
    std::cout << int3 << std::endl;

    Vec3 vec3{2.5, 1235.5134, 5245.2};
    std::cout << vec3 << std::endl;
    vec3 = vec3*5;
    std::cout << vec3 << std::endl;
    vec3 = vec3*2.1;
    std::cout << vec3 << std::endl;

    
    Byte2 b2;
    b2 = Byte2{5,16};
    std::cout << b2 << std::endl;

    // Test NBT example
    NBTexample::test();

    std::cout << "--All tests finished successfully.--\n";

    NetworkStream stream;
    if (stream.NewClient()) {
        std::cout << "Client connected!\n";
        std::wstring greeting = L"Hello from the server!";
        String16 username = String16(L"TestUser");
        PacketPreLogin packet(username);
        stream.Write(packet);
        stream.Write(greeting);
    } else {
        std::cout << "Failed to connect client.\n";
    }

    Client client;
    client.run();

    return 0;
}