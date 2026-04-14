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
#include "base_types.h"
#include "network_stream.h"
#include "numeric_structs.h"

// This class serves as a nice, convenient wrapper
// around the networking packets

class Packet {
    private:
    // NOTE: The base packet should never be used directly!!
    struct BasePacket {
        PacketId id;
        BasePacket(PacketId id) : id(id) {}
        virtual ~BasePacket() = default;
        virtual void Serialize(NetworkStream& stream) const = 0;
        virtual void Deserialize(NetworkStream& stream) = 0;
    };

    public:
    // Used to keep the connection alive
    struct KeepAlive : BasePacket {
        KeepAlive() : BasePacket{ PacketId::KeepAlive } {}

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
        }

        // NOTE: Reading the packet id is enough to deserialize it
        void Deserialize([[maybe_unused]] NetworkStream& stream) override {}
    };

    // Used to finalize the connection
    struct Login : BasePacket {
        Login() : BasePacket{ PacketId::Login } {}
        // NOTE: This assumes that EntityId is always an int32_t
        EntityId entity_id;
        EntityId& protocolVersion = entity_id;
        EntityId& entityId_protocolVersion = entity_id;

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

        void Deserialize(NetworkStream& stream) override {
            entityId_protocolVersion = stream.Read<EntityId>();
            username = stream.Read<std::string>();
            worldSeed = stream.Read<int64_t>();
            dimension = stream.Read<Dimension>();
        }
    };

    // Used to initialize to connection
    struct PreLogin : BasePacket {
        PreLogin() : BasePacket{ PacketId::PreLogin } {}
        std::string username;
        std::string& connection_hash = username;
        std::string& username_connectionHash = username;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(username_connectionHash);
        }

        void Deserialize(NetworkStream& stream) override {
            username_connectionHash = stream.Read<std::string>();
        }
    };

    // Holds a chat message
    struct ChatMessage : BasePacket {
        ChatMessage() : BasePacket{ PacketId::ChatMessage } {}
        std::string message;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(message);
        }

        void Deserialize(NetworkStream& stream) override {
            message = stream.Read<std::string>();
        }
    };

    // Holds the current time
    struct SetTime : BasePacket {
        SetTime() : BasePacket{ PacketId::SetTime } {}
        int64_t time;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(time);
        }

        void Deserialize(NetworkStream& stream) override {
            time = stream.Read<int64_t>();
        }
    };

    // Defines a players equipment
    struct SetEquipment : BasePacket {
        SetEquipment() : BasePacket{ PacketId::SetEquipment } {}
        EntityId entity_id;
        int16_t inventory_slot;
        int16_t item_id;
        int16_t item_metadata;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(inventory_slot);
            stream.Write(item_id);
            stream.Write(item_metadata);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            inventory_slot = stream.Read<int16_t>();
            item_id = stream.Read<int16_t>();
            item_metadata = stream.Read<int16_t>();
        }
    };

    // Defines where the compass points
    struct SetSpawnPosition : BasePacket {
        SetSpawnPosition() : BasePacket{ PacketId::SetSpawnPosition } {}
        Int32_3 position;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(position.z);
        }

        void Deserialize(NetworkStream& stream) override {
            position.x = stream.Read<int32_t>();
            position.y = stream.Read<int32_t>();
            position.y = stream.Read<int32_t>();
        }
    };

    // Used to convey who interacted with who
    struct InteractWithEntity : BasePacket {
        InteractWithEntity() : BasePacket{ PacketId::InteractWithEntity } {}
        EntityId source_entity_id;
        EntityId target_entity_id;
        bool left_click;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(source_entity_id);
            stream.Write(target_entity_id);
            stream.Write(left_click);
        }

        void Deserialize(NetworkStream& stream) override {
            source_entity_id = stream.Read<EntityId>();
            target_entity_id = stream.Read<EntityId>();
            left_click = stream.Read<bool>();
        }
    };

    // Defines a players health
    struct SetHealth : BasePacket {
        SetHealth() : BasePacket{ PacketId::SetHealth } {}
        int16_t health;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(health);
        }

        void Deserialize(NetworkStream& stream) override {
            health = stream.Read<int16_t>();
        }
    };

    // Defines a players health
    struct Respawn : BasePacket {
        Respawn() : BasePacket{ PacketId::Respawn } {}
        Dimension dimension;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(dimension);
        }

        void Deserialize(NetworkStream& stream) override {
            dimension = stream.Read<Dimension>();
        }
    };

    // Base Packet for player movement packets
    struct PlayerMovement : BasePacket {
        PlayerMovement() : BasePacket{ PacketId::PlayerMovement } {}
        bool on_ground;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(on_ground);
        }

        void Deserialize(NetworkStream& stream) override {
            on_ground = stream.Read<bool>();
        }
    };

    // Defines the players position
    struct PlayerPosition : BasePacket {
        PlayerPosition() : BasePacket{ PacketId::PlayerPosition } {}
        Vec3 position;
        double camera_y;
        bool on_ground;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(camera_y);
            stream.Write(position.z);
            stream.Write(on_ground);
        }

        void Deserialize(NetworkStream& stream) override {
            position.x = stream.Read<double>();
            position.y = stream.Read<double>();
            camera_y = stream.Read<double>();
            position.z = stream.Read<double>();
            on_ground = stream.Read<bool>();
        }
    };

    // Defines the players rotation
    struct PlayerRotation : BasePacket {
        PlayerRotation() : BasePacket{ PacketId::PlayerRotation } {}
        Float2 rotation;
        float& pitch = rotation.x;
        float& yaw = rotation.y;
        bool on_ground;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(pitch);
            stream.Write(yaw);
            stream.Write(on_ground);
        }

        void Deserialize(NetworkStream& stream) override {
            pitch = stream.Read<float>();
            yaw = stream.Read<float>();
            on_ground = stream.Read<bool>();
        }
    };

    // Defines the players position and rotation
    struct PlayerPositionAndRotation : BasePacket {
        PlayerPositionAndRotation() : BasePacket{ PacketId::PlayerPositionAndRotation } {}
        Vec3 position;
        double camera_y;
        Float2 rotation;
        float& pitch = rotation.x;
        float& yaw = rotation.y;
        bool onGround;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(camera_y);
            stream.Write(position.z);
            stream.Write(pitch);
            stream.Write(yaw);
            stream.Write(onGround);
        }

        void Deserialize(NetworkStream& stream) override {
            position.x = stream.Read<double>();
            position.y = stream.Read<double>();
            camera_y = stream.Read<double>();
            position.z = stream.Read<double>();
            pitch = stream.Read<float>();
            yaw = stream.Read<float>();
            onGround = stream.Read<bool>();
        }
    };
};