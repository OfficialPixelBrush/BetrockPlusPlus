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
#include <vector>
#include "packet_data.h"
#include "base_structs.h"
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
            position.z = stream.Read<int32_t>();
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
        double x = 0.0;
        double y = 0.0;
        double stance = 0.0;   // must be y + 1.62
        double z = 0.0;
        float  yaw = 0.0f;
        float  pitch = 0.0f;
        bool   onGround = false;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(x);
            stream.Write(y);
            stream.Write(stance);
            stream.Write(z);
            stream.Write(yaw);
            stream.Write(pitch);
            stream.Write(onGround);
        }

        void Deserialize(NetworkStream& stream) override {
            x = stream.Read<double>();
            y = stream.Read<double>();
            stance = stream.Read<double>();
            z = stream.Read<double>();
            yaw = stream.Read<float>();
            pitch = stream.Read<float>();
            onGround = stream.Read<bool>();
        }
    };

    // Information on how far along the player is with breaking a block
    struct MineBlock : BasePacket {
        MineBlock() : BasePacket{ PacketId::MineBlock } {}
        PacketData::MineStatus status;
        // TODO: Possibly use Int3?
        struct {
            int32_t x;
            int8_t y;
            int32_t z;
        } position;
        PacketData::FaceDirection face;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(status);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(position.z);
            stream.Write(face);
        }

        void Deserialize(NetworkStream& stream) override {
            status = stream.Read<PacketData::MineStatus>();
            position.x = stream.Read<int32_t>();
            position.y = stream.Read<int8_t>();
            position.z = stream.Read<int32_t>();
            face = stream.Read<PacketData::FaceDirection>();
        }
    };

    // Information on where a player is placing a block
    struct PlaceBlock : BasePacket {
        PlaceBlock() : BasePacket{ PacketId::PlaceBlock } {}
        // TODO: Possibly use Int3?
        struct {
            int32_t x;
            int8_t y;
            int32_t z;
        } position;
        PacketData::FaceDirection face;
        Item item;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(position.z);
            stream.Write(face);
            stream.Write(item.id);
            stream.Write(item.amount);
            stream.Write(item.damage);
        }

        void Deserialize(NetworkStream& stream) override {
            position.x = stream.Read<int32_t>();
            position.y = stream.Read<int8_t>();
            position.z = stream.Read<int32_t>();
            face = stream.Read<PacketData::FaceDirection>();
            item.id = stream.Read<ItemId>();
            item.amount = stream.Read<ItemAmount>();
            item.damage = stream.Read<ItemDamage>();
        }
    };

    // The clients active hotbar slot
    struct SetHotbarSlot : BasePacket {
        SetHotbarSlot() : BasePacket{ PacketId::SetHotbarSlot } {}
        int16_t slot;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(slot);
        }

        void Deserialize(NetworkStream& stream) override {
            slot = stream.Read<int16_t>();
        }
    };

    // Interactions with blocks
    struct InteractWithBlock : BasePacket {
        InteractWithBlock() : BasePacket{ PacketId::InteractWithBlock } {}
        EntityId entity_id;
        PacketData::BlockInteraction interaction_id;
        struct {
            int32_t x;
            int8_t y;
            int32_t z;
        } position;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(interaction_id);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(position.z);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            interaction_id = stream.Read<PacketData::BlockInteraction>();
            position.x = stream.Read<int32_t>();
            position.y = stream.Read<int8_t>();
            position.z = stream.Read<int32_t>();
        }
    };

    // Informs of the desired animation
    struct Animation : BasePacket {
        Animation() : BasePacket{ PacketId::Animation } {}
        EntityId entity_id;
        PacketData::Animation animation;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(animation);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            animation = stream.Read<PacketData::Animation>();
        }
    };

    // Used for simple actions, like sneaking
    struct PlayerAction : BasePacket {
        PlayerAction() : BasePacket{ PacketId::PlayerAction } {}
        EntityId entity_id;
        PacketData::PlayerAction action;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(action);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            action = stream.Read<PacketData::PlayerAction>();
        }
    };

    // Used for spawning other players in the world
    struct SpawnPlayer : BasePacket {
        SpawnPlayer() : BasePacket{ PacketId::SpawnPlayer } {}
        EntityId entity_id;
        std::string username;
        Int32_3 q_position;
        Int8_2 q_rotation;
        int8_t& q_pitch = q_rotation.x;
        int8_t& q_yaw = q_rotation.y;
        ItemId held_item_id;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(username);
            stream.Write(q_position.x);
            stream.Write(q_position.y);
            stream.Write(q_position.z);
            stream.Write(q_pitch);
            stream.Write(q_yaw);
            stream.Write(held_item_id);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            username = stream.Read<std::string>();
            q_position.x = stream.Read<int32_t>();
            q_position.y = stream.Read<int32_t>();
            q_position.z = stream.Read<int32_t>();
            q_pitch = stream.Read<int8_t>();
            q_yaw = stream.Read<int8_t>();
            held_item_id = stream.Read<ItemId>();
        }
    };

    // Used for spawning items in the world
    struct SpawnItem : BasePacket {
        SpawnItem() : BasePacket{ PacketId::SpawnItem } {}
        EntityId entity_id;
        Item item;
        Int32_3 q_position;
        Int8_3 q_rotation;
        int8_t& q_pitch = q_rotation.x;
        int8_t& q_yaw = q_rotation.y;
        int8_t& q_roll = q_rotation.z;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(item.id);
            stream.Write(item.amount);
            stream.Write(item.damage);
            stream.Write(q_position.x);
            stream.Write(q_position.y);
            stream.Write(q_position.z);
            stream.Write(q_pitch);
            stream.Write(q_yaw);
            stream.Write(q_roll);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            item.id = stream.Read<ItemId>();
            item.amount = stream.Read<ItemAmount>();
            item.damage = stream.Read<ItemDamage>();
            q_position.x = stream.Read<int32_t>();
            q_position.y = stream.Read<int32_t>();
            q_position.z = stream.Read<int32_t>();
            q_pitch = stream.Read<int8_t>();
            q_yaw = stream.Read<int8_t>();
            q_roll = stream.Read<int8_t>();
        }
    };

    // Used for collecting items visually
    struct CollectItem : BasePacket {
        CollectItem() : BasePacket{ PacketId::CollectItem } {}
        EntityId item_entity_id;
        EntityId collector_entity_id;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(item_entity_id);
            stream.Write(collector_entity_id);
        }

        void Deserialize(NetworkStream& stream) override {
            item_entity_id = stream.Read<EntityId>();
            collector_entity_id = stream.Read<EntityId>();
        }
    };

    // Used for spawning objects in the world
    struct SpawnObject : BasePacket {
        SpawnObject() : BasePacket{ PacketId::SpawnObject } {}
        EntityId entity_id;
        PacketData::ObjectType object_type;
        Int32_3 q_position;
        EntityId owner_entity_id = 0;
        Int16_3 q_velocity;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(object_type);
            stream.Write(q_position.x);
            stream.Write(q_position.y);
            stream.Write(q_position.z);
            stream.Write(owner_entity_id);
            if (owner_entity_id) {
                stream.Write(q_velocity.x);
                stream.Write(q_velocity.y);
                stream.Write(q_velocity.z);
            }
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            object_type = stream.Read<PacketData::ObjectType>();
            q_position.x = stream.Read<int32_t>();
            q_position.y = stream.Read<int32_t>();
            q_position.z = stream.Read<int32_t>();
            owner_entity_id = stream.Read<EntityId>();
            if (owner_entity_id) {
                q_velocity.x = stream.Read<int16_t>();
                q_velocity.y = stream.Read<int16_t>();
                q_velocity.z = stream.Read<int16_t>();
            }
        }
    };

    // Used for spawning mobs in the world
    struct SpawnMob : BasePacket {
        SpawnMob() : BasePacket{ PacketId::SpawnMob } {}
        EntityId entity_id;
        PacketData::MobType mob_type;
        Int32_3 q_position;
        Int8_2 q_rotation;
        int8_t& q_pitch = q_rotation.x;
        int8_t& q_yaw = q_rotation.y;
        //std::vector<uint8_t> metadata;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(mob_type);
            stream.Write(q_position.x);
            stream.Write(q_position.y);
            stream.Write(q_position.z);
            stream.Write(q_pitch);
            stream.Write(q_yaw);
            // TODO: Metadata handling
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            mob_type = stream.Read<PacketData::MobType>();
            q_position.x = stream.Read<int32_t>();
            q_position.y = stream.Read<int32_t>();
            q_position.z = stream.Read<int32_t>();
            q_pitch = stream.Read<int8_t>();
            q_yaw = stream.Read<int8_t>();
            // TODO: Metadata handling
        }
    };

    // Used for spawning paintings in the world
    struct SpawnPainting : BasePacket {
        SpawnPainting() : BasePacket{ PacketId::SpawnPainting } {}
        EntityId entity_id;
        std::string title;
        Int32_3 position; // Block position
        PacketData::PaintingDirection direction;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(title);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(position.z);
            stream.Write(direction);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            title = stream.Read<std::string>();
            position.x = stream.Read<int32_t>();
            position.y = stream.Read<int32_t>();
            position.z = stream.Read<int32_t>();
            direction = stream.Read<PacketData::PaintingDirection>();
        }
    };

    // Unused, but exists for sending raw player input to the client/server
    struct PlayerInput : BasePacket {
        PlayerInput() : BasePacket{ PacketId::PlayerInput } {}
        Float2 direction;
        float& strafe_direction = direction.x;
        float& forward_direction = direction.y;
        Float2 rotation;
        float& pitch = rotation.x;
        float& yaw = rotation.y;
        bool jumping;
        bool sneaking;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(strafe_direction);
            stream.Write(forward_direction);
            stream.Write(pitch);
            stream.Write(yaw);
            stream.Write(jumping);
            stream.Write(sneaking);
        }

        void Deserialize(NetworkStream& stream) override {
            strafe_direction = stream.Read<float>();
            forward_direction = stream.Read<float>();
            pitch = stream.Read<float>();
            yaw = stream.Read<float>();
            jumping = stream.Read<bool>();
            sneaking = stream.Read<bool>();
        }
    };

    // Used to update an entities velocity
    struct EntityVelocity : BasePacket {
        EntityVelocity() : BasePacket{ PacketId::EntityVelocity } {}
        EntityId entity_id;
        Int16_3 velocity;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(velocity.x);
            stream.Write(velocity.y);
            stream.Write(velocity.z);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            velocity.x = stream.Read<int16_t>();
            velocity.y = stream.Read<int16_t>();
            velocity.z = stream.Read<int16_t>();
        }
    };

    // Used to despawn an entity
    struct DespawnEntity : BasePacket {
        DespawnEntity() : BasePacket{ PacketId::DespawnEntity } {}
        EntityId entity_id;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
        }
    };

    // Unused, Base Packet for entity movement packets
    struct EntityMovement : BasePacket {
        EntityMovement() : BasePacket{ PacketId::EntityMovement } {}

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
        }

        void Deserialize([[maybe_unused]]NetworkStream& stream) override {
        }
    };

    // Used for setting an entitys relative position
    struct EntityPosition : BasePacket {
        EntityPosition() : BasePacket{ PacketId::EntityPosition } {}
        EntityId entity_id;
        Int8_3 qr_position;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(qr_position.x);
            stream.Write(qr_position.y);
            stream.Write(qr_position.z);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            qr_position.x = stream.Read<int8_t>();
            qr_position.y = stream.Read<int8_t>();
            qr_position.z = stream.Read<int8_t>();
        }
    };

    // Used for setting an entitys rotation
    struct EntityRotation : BasePacket {
        EntityRotation() : BasePacket{ PacketId::EntityRotation } {}
        EntityId entity_id;
        Int8_2 q_rotation;
        int8_t& q_pitch = q_rotation.x;
        int8_t& q_yaw = q_rotation.y;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(q_pitch);
            stream.Write(q_yaw);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            q_pitch = stream.Read<int8_t>();
            q_yaw = stream.Read<int8_t>();
        }
    };

    // Used for setting an entitys relative position and rotation
    struct EntityPositionAndRotation : BasePacket {
        EntityPositionAndRotation() : BasePacket{ PacketId::EntityPositionAndRotation } {}
        EntityId entity_id;
        Int8_3 qr_position;
        Int8_2 q_rotation;
        int8_t& q_pitch = q_rotation.x;
        int8_t& q_yaw = q_rotation.y;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(qr_position.x);
            stream.Write(qr_position.y);
            stream.Write(qr_position.z);
            stream.Write(q_pitch);
            stream.Write(q_yaw);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            qr_position.x = stream.Read<int8_t>();
            qr_position.y = stream.Read<int8_t>();
            qr_position.z = stream.Read<int8_t>();
            q_pitch = stream.Read<int8_t>();
            q_yaw = stream.Read<int8_t>();
        }
    };

    // Used for setting an entitys absolute position
    struct TeleportEntity : BasePacket {
        TeleportEntity() : BasePacket{ PacketId::TeleportEntity } {}
        EntityId entity_id;
        Int32_3 position;
        Int8_2 rotation;
        int8_t& pitch = rotation.x;
        int8_t& yaw = rotation.y;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(position.z);
            stream.Write(pitch);
            stream.Write(yaw);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            position.x = stream.Read<int32_t>();
            position.y = stream.Read<int32_t>();
            position.z = stream.Read<int32_t>();
            pitch = stream.Read<int8_t>();
            yaw = stream.Read<int8_t>();
        }
    };

    // Used for some entity animations
    struct EntityEvent : BasePacket {
        EntityEvent() : BasePacket{ PacketId::EntityEvent } {}
        EntityId entity_id;
        PacketData::EntityEvent action;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(action);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            action = stream.Read<PacketData::EntityEvent>();
        }
    };

    // Used for mounting and dismounting entities
    struct AddPassenger : BasePacket {
        AddPassenger() : BasePacket{ PacketId::AddPassenger } {}
        EntityId passenger_entity_id;
        EntityId vehicle_entity_id;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(passenger_entity_id);
            stream.Write(vehicle_entity_id);
        }

        void Deserialize(NetworkStream& stream) override {
            passenger_entity_id = stream.Read<EntityId>();
            vehicle_entity_id = stream.Read<EntityId>();
        }
    };

    // Used for mounting and dismounting entities
    struct EntityMetadata : BasePacket {
        EntityMetadata() : BasePacket{ PacketId::EntityMetadata } {}
        EntityId entity_id;
        std::vector<uint8_t> metadata;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            // TODO: Metadata handling
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            // TODO: Metadata handling
        }
    };

    // Tells the client to allocate or free a chunk slot — must be sent before ChunkData
    struct SetChunkVisibility : BasePacket {
        SetChunkVisibility() : BasePacket{ PacketId::SetChunkVisibility } {}
        int32_t chunkX;
        int32_t chunkZ;
        bool visible;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(chunkX);
            stream.Write(chunkZ);
            stream.Write(visible);
        }

        void Deserialize(NetworkStream& stream) override {
            chunkX = stream.Read<int32_t>();
            chunkZ = stream.Read<int32_t>();
            visible = stream.Read<bool>();
        }
    };

    // Sends compressed chunk data; always preceded by SetChunkVisibility
    struct ChunkData : BasePacket {
        ChunkData() : BasePacket{ PacketId::Chunk } {}
        int32_t chunkX;
        int16_t chunkY = 0;    // always 0 for a full-height chunk
        int32_t chunkZ;
        uint8_t sizeX = 15;   // sent as (size - 1), so 15 = 16 wide
        uint8_t sizeY = 127;  // 127 = 128 tall
        uint8_t sizeZ = 15;
        std::vector<uint8_t> compressedData;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(chunkX);
            stream.Write(chunkY);
            stream.Write(chunkZ);
            stream.Write(sizeX);
            stream.Write(sizeY);
            stream.Write(sizeZ);
            stream.Write(static_cast<int32_t>(compressedData.size()));
            stream.WriteBytes(compressedData.data(), compressedData.size());
        }

        void Deserialize(NetworkStream& stream) override {
            chunkX = stream.Read<int32_t>();
            chunkY = stream.Read<int16_t>();
            chunkZ = stream.Read<int32_t>();
            sizeX = stream.Read<uint8_t>();
            sizeY = stream.Read<uint8_t>();
            sizeZ = stream.Read<uint8_t>();
            int32_t size = stream.Read<int32_t>();
            compressedData.resize(static_cast<size_t>(size));
            stream.ReadBytes(compressedData.data(), static_cast<size_t>(size));
        }
    };

    // Used to set multiple blocks in a small area
    struct SetMultipleBlocks : BasePacket {
        SetMultipleBlocks() : BasePacket{ PacketId::SetMultipleBlocks } {}
        Int32_2 chunk_position;
        int16_t number_of_blocks;
        std::vector<int16_t> block_coordinates;
        std::vector<BlockType> block_types;
        std::vector<int8_t> block_metadata; // Nibbles

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(chunk_position.x);
            stream.Write(chunk_position.z);
            stream.Write(number_of_blocks);
            for (int16_t i = 0; i < number_of_blocks; i++)
                stream.Write(block_coordinates[static_cast<size_t>(i)]);
            for (int16_t i = 0; i < number_of_blocks; i++)
                stream.Write(block_types[static_cast<size_t>(i)]);
            for (int16_t i = 0; i < number_of_blocks; i++)
                stream.Write(block_metadata[static_cast<size_t>(i)]);
        }

        void Deserialize(NetworkStream& stream) override {
            chunk_position.x = stream.Read<int32_t>();
            chunk_position.z = stream.Read<int32_t>();
            number_of_blocks = stream.Read<int16_t>();
            block_coordinates.resize(static_cast<size_t>(number_of_blocks));
            block_types.resize(static_cast<size_t>(number_of_blocks));
            block_metadata.resize(static_cast<size_t>(number_of_blocks));
            for (int16_t i = 0; i < number_of_blocks; i++)
                block_coordinates[static_cast<size_t>(i)] = stream.Read<int16_t>();
            for (int16_t i = 0; i < number_of_blocks; i++)
                block_types[static_cast<size_t>(i)] = stream.Read<BlockType>();
            for (int16_t i = 0; i < number_of_blocks; i++)
                block_metadata[static_cast<size_t>(i)] = stream.Read<int8_t>();
        }
    };

    // Used to set a singular block
    struct SetBlock : BasePacket {
        SetBlock() : BasePacket{ PacketId::SetBlock } {}
        struct {
            int32_t x;
            int8_t y;
            int32_t z;
        } position;
        Block block;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(position.z);
            stream.Write(block.type);
            stream.Write(block.data);
        }

        void Deserialize(NetworkStream& stream) override {
            position.x = stream.Read<int32_t>();
            position.y = stream.Read<int8_t>();
            position.z = stream.Read<int32_t>();
            block.type = stream.Read<BlockType>();
            block.data = stream.Read<int8_t>();
        }
    };

    // Used to set a singular block
    struct BlockEvent : BasePacket {
        BlockEvent() : BasePacket{ PacketId::BlockEvent } {}
        struct {
            int32_t x;
            int8_t y;
            int32_t z;
        } position;
        int8_t instrument_state;
        int8_t pitch_direction;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(position.z);
            stream.Write(instrument_state);
            stream.Write(pitch_direction);
        }

        void Deserialize(NetworkStream& stream) override {
            position.x = stream.Read<int32_t>();
            position.y = stream.Read<int8_t>();
            position.z = stream.Read<int32_t>();
            instrument_state = stream.Read<int8_t>();
            pitch_direction = stream.Read<int8_t>();
        }

        PacketData::NoteInstrument instrument() const {
            return static_cast<PacketData::NoteInstrument>(instrument_state);
        }

        PacketData::NotePitch pitch() const {
            return static_cast<PacketData::NotePitch>(pitch_direction);
        }

        PacketData::PistonState state() const {
            return static_cast<PacketData::PistonState>(instrument_state);
        }

        PacketData::PistonDirection direction() const {
            return static_cast<PacketData::PistonDirection>(pitch_direction);
        }
    };

    // Used for explosions
    struct Explosion : BasePacket {
        Explosion() : BasePacket{ PacketId::Explosion } {}
        Vec3 position;
        float radius;
        int32_t number_of_destroyed_blocks;
        std::vector<int8_t> destroyed_blocks;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(position.z);
            stream.Write(radius);
            stream.Write(static_cast<int32_t>(destroyed_blocks.size()));
            stream.WriteBytes(reinterpret_cast<const uint8_t*>(destroyed_blocks.data()), destroyed_blocks.size());
        }

        void Deserialize(NetworkStream& stream) override {
            position.x = stream.Read<double>();
            position.y = stream.Read<double>();
            position.z = stream.Read<double>();
            radius = stream.Read<float>();
            number_of_destroyed_blocks = stream.Read<int32_t>();
            destroyed_blocks.resize(static_cast<size_t>(number_of_destroyed_blocks));
            stream.ReadBytes(reinterpret_cast<uint8_t*>(destroyed_blocks.data()), static_cast<size_t>(number_of_destroyed_blocks));
        }
    };

    struct WorldEvent : BasePacket {
        WorldEvent() : BasePacket{ PacketId::WorldEvent } {}
        PacketData::WorldEvent event_id;
        struct {
            int32_t x;
            int8_t y;
            int32_t z;
        } position;
        int32_t data;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(event_id);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(position.z);
            stream.Write(data);
        }

        void Deserialize(NetworkStream& stream) override {
            event_id = stream.Read<PacketData::WorldEvent>();
            position.x = stream.Read<int32_t>();
            position.y = stream.Read<int8_t>();
            position.z = stream.Read<int32_t>();
            data = stream.Read<int32_t>();
        }
    };

    struct GameEvent : BasePacket {
        GameEvent() : BasePacket{ PacketId::GameEvent } {}
        PacketData::GameEvent event_id;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(event_id);
        }

        void Deserialize(NetworkStream& stream) override {
            event_id = stream.Read<PacketData::GameEvent>();
        }
    };

    struct LightningBolt : BasePacket {
        LightningBolt() : BasePacket{ PacketId::LightningBolt } {}
        EntityId entity_id;
        // This is only ever "1", which means lightning
        int8_t entity_type = 1;
        Int32_3 position;

        void Serialize(NetworkStream& stream) const override {
            stream.Write(id);
            stream.Write(entity_id);
            stream.Write(entity_type);
            stream.Write(position.x);
            stream.Write(position.y);
            stream.Write(position.z);
        }

        void Deserialize(NetworkStream& stream) override {
            entity_id = stream.Read<EntityId>();
            entity_type = stream.Read<int8_t>();
            position.x = stream.Read<int32_t>();
            position.y = stream.Read<int32_t>();
            position.z = stream.Read<int32_t>();
        }
    };
};