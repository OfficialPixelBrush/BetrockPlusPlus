#include "network_test.h"
#include "../bpp_shared/networking/network_stream.h"
#include "../bpp_shared/networking/packets.h"
#include "../networking/server_manager.h"
#include <iostream>

int ServerTest() {
    std::cout << "Network test, waiting for client...\n";
    ServerManager sm;
    if (!sm.InitConnection()) {
        std::cerr << "Failed to connect client.\n";
        return 1;
    }
    std::cout << "Client connected!\n";

    NetworkStream stream = sm.streams[0];

    // C -> S
    std::cout << int(stream.Read<PacketId>()) << std::endl;;
    Packet::PreLogin preLogin_client;
    preLogin_client.Deserialize(stream);

    std::cout << "Received PreLogin packet from client: " << preLogin_client.username << "\n";

    // S -> C
    Packet::PreLogin preLogin_server;
    preLogin_server.connection_hash = "-";
    preLogin_server.Serialize(stream);

    // C -> S
    std::cout << int(stream.Read<PacketId>()) << std::endl;;
    Packet::Login login_client;
    login_client.Deserialize(stream);

    std::cout << "Received Login packet from client: " << login_client.username << "\n";

    // S -> C
    Packet::Login login_server;
    login_server.entity_id = 0;
    login_server.username = "";
    login_server.worldSeed = 123456789;
    login_server.dimension = Dimension::Overworld;
    login_server.Serialize(stream);

    // S -> C
    Packet::PlayerPositionAndRotation posRot_server;
    posRot_server.position.x = 0.0;
    posRot_server.position.y = 1.0;
    posRot_server.camera_y = 2.0;
    posRot_server.position.z = 4.0;
    posRot_server.pitch = 45.0f;
    posRot_server.yaw = 45.0f;
    posRot_server.onGround = true;
    posRot_server.Serialize(stream);
    return 0;
}