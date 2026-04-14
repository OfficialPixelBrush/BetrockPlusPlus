/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <string>
#include <dimensions.h>
#include <packet_ids.h>
#include <string16.h>

struct Packet {
    PacketId id;
};

struct PacketKeepAlive : Packet {
    PacketKeepAlive() : Packet{ PacketId::KeepAlive } {}
};

struct PacketLogin : Packet {
    PacketLogin() : Packet{ PacketId::Login } {}
    union {
        int32_t entityId;
        int32_t protocolVersion;
    };
    String16 username;
    int64_t worldSeed;
    Dimension dimension;
};

struct PacketPreLogin : Packet {
    PacketPreLogin() : Packet{ PacketId::PreLogin } {}
    union {
        String16 username;
        String16 connectionHash;
    };
    
};

struct PacketChatMessage : Packet {
    PacketChatMessage() : Packet{ PacketId::ChatMessage } {}
    String16 message;
};

struct PacketPlayerPositionAndRotation : Packet {
    PacketPlayerPositionAndRotation() : Packet{ PacketId::PlayerPositionAndRotation } {}
    double x, y, camera_y, z;
    float yaw, pitch;
    bool onGround;
};