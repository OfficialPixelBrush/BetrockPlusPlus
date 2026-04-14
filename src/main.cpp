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
    bool networkTest = true;
    #if defined(_WIN32) || defined(_WIN64)
        std::cout << "Running on Windows\n";
        networkTest = false;
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

    if (networkTest) {
        std::cout << "Network test, waiting for client...\n";
        NetworkStream stream;
        if (stream.NewClient()) {
            std::cout << "Client connected!\n";

            // C -> S
            std::cout << int(stream.Read<PacketId>()) << std::endl;;
            PacketPreLogin preLogin_client;
            preLogin_client.Deserialize(stream);

            std::cout << "Received PreLogin packet from client: " << preLogin_client.username << "\n";

            // S -> C
            PacketPreLogin preLogin_server;
            preLogin_server.connection_hash = "-";
            preLogin_server.Serialize(stream);

            // C -> S
            std::cout << int(stream.Read<PacketId>()) << std::endl;;
            PacketLogin login_client;
            login_client.Deserialize(stream);

            std::cout << "Received Login packet from client: " << login_client.username << "\n";

            // S -> C
            PacketLogin login_server;
            login_server.entityId = 0;
            login_server.username = "";
            login_server.worldSeed = 123456789;
            login_server.dimension = Dimension::Overworld;
            login_server.Serialize(stream);

            // S -> C
            PacketPlayerPositionAndRotation posRot_server;
            posRot_server.x = 0.0;
            posRot_server.y = 1.0;
            posRot_server.camera_y = 2.0;
            posRot_server.z = 4.0;
            posRot_server.yaw = 90.0f;
            posRot_server.pitch = 45.0f;
            posRot_server.onGround = true;
            posRot_server.Serialize(stream);
        } else {
            std::cout << "Failed to connect client.\n";
        }
    }

    Client client;
    client.run();

    return 0;
}