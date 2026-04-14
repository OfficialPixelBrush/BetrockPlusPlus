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
#include "network_stream.h"

struct Packet {
    PacketId id;
    Packet(PacketId id) : id(id) {}
    virtual ~Packet() = default;
    virtual void Serialize(NetworkStream& stream) const = 0;
};

struct PacketKeepAlive : Packet {
    PacketKeepAlive() : Packet{ PacketId::KeepAlive } {}

    void Serialize(NetworkStream& stream) const override {
        stream.Write(id);
    }
};

struct PacketLogin : Packet {
    PacketLogin() : Packet{ PacketId::Login } {}
    int32_t entityId;
    int32_t& protocolVersion = entityId;
    int32_t& entityId_protocolVersion = entityId;

    std::string username;
    int64_t worldSeed;
    Dimension dimension;

    void Serialize(NetworkStream& stream) const override {
        stream.Write(id);
        stream.Write(entityId_protocolVersion);
        stream.Write(username);
        stream.Write(worldSeed);
        stream.Write(dimension);
    }

    void Deserialize(NetworkStream& stream) {
        entityId_protocolVersion = stream.Read<int32_t>();
        username = stream.Read<std::string>();
        worldSeed = stream.Read<int64_t>();
        dimension = stream.Read<Dimension>();
    }
};

struct PacketPreLogin : Packet {
    PacketPreLogin() : Packet{ PacketId::PreLogin } {}
    std::string username;
    std::string& connection_hash = username;
    std::string& username_connectionHash = username;

    void Serialize(NetworkStream& stream) const override {
        stream.Write(id);
        stream.Write(username_connectionHash);
    }

    void Deserialize(NetworkStream& stream) {
        username_connectionHash = stream.Read<std::string>();
    }
};

struct PacketChatMessage : Packet {
    PacketChatMessage() : Packet{ PacketId::ChatMessage } {}
    std::string message;

    void Serialize(NetworkStream& stream) const override {
        stream.Write(id);
        stream.Write(message);
    }

    void Deserialize(NetworkStream& stream) {
        message = stream.Read<std::string>();
    }
};

struct PacketPlayerPositionAndRotation : Packet {
    PacketPlayerPositionAndRotation() : Packet{ PacketId::PlayerPositionAndRotation } {}
    double x, y, camera_y, z;
    float yaw, pitch;
    bool onGround;

    void Serialize(NetworkStream& stream) const override {
        stream.Write(id);
        stream.Write(x);
        stream.Write(y);
        stream.Write(camera_y);
        stream.Write(z);
        stream.Write(yaw);
        stream.Write(pitch);
        stream.Write(onGround);
    }

    void Deserialize(NetworkStream& stream) {
        x = stream.Read<double>();
        y = stream.Read<double>();
        camera_y = stream.Read<double>();
        z = stream.Read<double>();
        yaw = stream.Read<float>();
        pitch = stream.Read<float>();
        onGround = stream.Read<bool>();
    }
};