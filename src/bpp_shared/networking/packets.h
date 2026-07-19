/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "base_structs.h"
#include "base_types.h"
#include "constants.h"
#include "inventory/item_stack.h"
#include "network_stream.h"
#include "numeric_structs.h"
#include "packet_data.h"
#include "quantized_types.h"
#include <dimensions.h>
#include <packet_ids.h>
#include <string>
#include <vector>

// This class serves as a nice, convenient wrapper
// around the networking packets

class Packet {
public:
	// NOTE: The base packet should never be used directly!!
	// Only public so that packets can be passed through functions
	struct BasePacket {
		PacketId m_id;
		BasePacket(PacketId id) : m_id(id) {}
		virtual ~BasePacket() = default;
		virtual void Serialize(NetworkStream& stream) const = 0;
		virtual void Deserialize(NetworkStream& stream) = 0;
	};

	// Used to keep the connection alive
	struct KeepAlive : BasePacket {
		KeepAlive() : BasePacket{ PacketId::KeepAlive } {}

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
		}

		// NOTE: Reading the packet id is enough to deserialize it
		void Deserialize([[maybe_unused]] NetworkStream& stream) override {}
	};

	// Used to finalize the connection
	struct Login : BasePacket {
		Login() : BasePacket{ PacketId::Login } {}
		// NOTE: This assumes that EntityId is always an int32_t
		EntityId m_entity_id;
		EntityId& m_protocolVersion = m_entity_id;
		EntityId& m_entityId_protocolVersion = m_entity_id;

		std::string m_username;
		int64_t m_worldSeed;
		Dimension m_dimension;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entityId_protocolVersion);
			stream.WriteString16(m_username);
			stream.Write(m_worldSeed);
			stream.Write(m_dimension);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entityId_protocolVersion = stream.Read<EntityId>();
			m_username = stream.ReadString16();
			m_worldSeed = stream.Read<int64_t>();
			m_dimension = stream.Read<Dimension>();
		}
	};

	// Used to initialize to connection
	struct PreLogin : BasePacket {
		PreLogin() : BasePacket{ PacketId::PreLogin } {}
		std::string m_username;
		std::string& m_connection_hash = m_username;
		std::string& m_username_connectionHash = m_username;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.WriteString16(m_username_connectionHash);
		}

		void Deserialize(NetworkStream& stream) override {
			m_username_connectionHash = stream.ReadString16();
		}
	};

	// Holds a chat message
	struct ChatMessage : BasePacket {
		ChatMessage() : BasePacket{ PacketId::ChatMessage } {}
		std::string m_message;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.WriteString16(m_message);
		}

		void Deserialize(NetworkStream& stream) override {
			m_message = stream.ReadString16();
		}
	};

	// Holds the current time
	struct SetTime : BasePacket {
		SetTime() : BasePacket{ PacketId::SetTime } {}
		TickTime m_time;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_time);
		}

		void Deserialize(NetworkStream& stream) override {
			m_time = stream.Read<TickTime>();
		}
	};

	// Defines a players equipment
	struct SetEquipment : BasePacket {
		SetEquipment() : BasePacket{ PacketId::SetEquipment } {}
		EntityId m_entity_id;
		NetworkSlotId m_inventory_slot;
		ItemId m_item_id;
		ItemDamage m_item_metadata;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_inventory_slot);
			stream.Write(m_item_id);
			stream.Write(m_item_metadata);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_inventory_slot = stream.Read<NetworkSlotId>();
			m_item_id = stream.Read<ItemId>();
			m_item_metadata = stream.Read<ItemDamage>();
		}
	};

	// Defines where the compass points
	struct SetSpawnPosition : BasePacket {
		SetSpawnPosition() : BasePacket{ PacketId::SetSpawnPosition } {}
		Int32_3 m_position;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_position.m_z);
		}

		void Deserialize(NetworkStream& stream) override {
			m_position.m_x = stream.Read<int32_t>();
			m_position.m_y = stream.Read<int32_t>();
			m_position.m_z = stream.Read<int32_t>();
		}
	};

	// Used to convey who interacted with who
	struct InteractWithEntity : BasePacket {
		InteractWithEntity() : BasePacket{ PacketId::InteractWithEntity } {}
		EntityId m_source_entity_id;
		EntityId m_target_entity_id;
		bool m_attack; // Usually sent when left-clicking

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_source_entity_id);
			stream.Write(m_target_entity_id);
			stream.Write(m_attack);
		}

		void Deserialize(NetworkStream& stream) override {
			m_source_entity_id = stream.Read<EntityId>();
			m_target_entity_id = stream.Read<EntityId>();
			m_attack = stream.Read<bool>();
		}
	};

	// Defines a players health
	struct SetHealth : BasePacket {
		SetHealth() : BasePacket{ PacketId::SetHealth } {}
		int16_t m_health;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_health);
		}

		void Deserialize(NetworkStream& stream) override {
			m_health = stream.Read<int16_t>();
		}
	};

	// Defines a players health
	struct Respawn : BasePacket {
		Respawn() : BasePacket{ PacketId::Respawn } {}
		Dimension m_dimension;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_dimension);
		}

		void Deserialize(NetworkStream& stream) override {
			m_dimension = stream.Read<Dimension>();
		}
	};

	// Base Packet for player movement packets
	struct PlayerMovement : BasePacket {
		PlayerMovement() : BasePacket{ PacketId::PlayerMovement } {}
		bool m_on_ground;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_on_ground);
		}

		void Deserialize(NetworkStream& stream) override {
			m_on_ground = stream.Read<bool>();
		}
	};

	// Defines the players position
	struct PlayerPosition : BasePacket {
		PlayerPosition() : BasePacket{ PacketId::PlayerPosition } {}
		Vec3 m_position;
		double m_camera_y;
		bool m_on_ground;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_camera_y);
			stream.Write(m_position.m_z);
			stream.Write(m_on_ground);
		}

		void Deserialize(NetworkStream& stream) override {
			m_position.m_x = stream.Read<double>();
			m_position.m_y = stream.Read<double>();
			m_camera_y = stream.Read<double>();
			m_position.m_z = stream.Read<double>();
			m_on_ground = stream.Read<bool>();
		}
	};

	// Defines the players rotation
	struct PlayerRotation : BasePacket {
		PlayerRotation() : BasePacket{ PacketId::PlayerRotation } {}
		Float2 m_rotation;
		float& m_yaw = m_rotation.m_x; // wire order: yaw first
		float& m_pitch = m_rotation.m_y;
		bool m_on_ground;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_yaw);
			stream.Write(m_pitch);
			stream.Write(m_on_ground);
		}

		void Deserialize(NetworkStream& stream) override {
			m_yaw = stream.Read<float>();
			m_pitch = stream.Read<float>();
			m_on_ground = stream.Read<bool>();
		}
	};

	// Defines the players position and rotation
	struct PlayerPositionAndRotation : BasePacket {
		PlayerPositionAndRotation() : BasePacket{ PacketId::PlayerPositionAndRotation } {}
		Vec3 m_position;
		double m_camera_y;
		Float2 m_rotation;
		float& m_yaw = m_rotation.m_x; // wire order: yaw first
		float& m_pitch = m_rotation.m_y;
		bool m_onGround = false;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_camera_y);
			stream.Write(m_position.m_z);
			stream.Write(m_yaw);
			stream.Write(m_pitch);
			stream.Write(m_onGround);
		}

		void Deserialize(NetworkStream& stream) override {
			m_position.m_x = stream.Read<double>();
			m_position.m_y = stream.Read<double>();
			m_camera_y = stream.Read<double>();
			m_position.m_z = stream.Read<double>();
			m_yaw = stream.Read<float>();
			m_pitch = stream.Read<float>();
			m_onGround = stream.Read<bool>();
		}
	};

	// Information on how far along the player is with breaking a block
	struct MineBlock : BasePacket {
		MineBlock() : BasePacket{ PacketId::MineBlock } {}
		PacketData::MineStatus m_status;
		SlimInt3<int8_t> m_position;
		PacketData::FaceDirection m_face;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_status);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_position.m_z);
			stream.Write(m_face);
		}

		void Deserialize(NetworkStream& stream) override {
			m_status = stream.Read<PacketData::MineStatus>();
			m_position.m_x = stream.Read<int32_t>();
			m_position.m_y = stream.Read<int8_t>();
			m_position.m_z = stream.Read<int32_t>();
			m_face = stream.Read<PacketData::FaceDirection>();
		}
	};

	// Information on where a player is placing a block
	struct PlaceBlock : BasePacket {
		PlaceBlock() : BasePacket{ PacketId::PlaceBlock } {}
		SlimInt3<int8_t> m_position;
		PacketData::FaceDirection m_face;
		ItemStack m_item;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_position.m_z);
			stream.Write(m_face);
			stream.Write(m_item.m_id);
			stream.Write(m_item.m_count);
			stream.Write(m_item.m_data);
		}

		void Deserialize(NetworkStream& stream) override {
			m_position.m_x = stream.Read<int32_t>();
			m_position.m_y = stream.Read<int8_t>();
			m_position.m_z = stream.Read<int32_t>();
			m_face = stream.Read<PacketData::FaceDirection>();
			m_item.m_id = stream.Read<ItemId>();
			if (static_cast<int16_t>(m_item.m_id) >= 0) {
				m_item.m_count = stream.Read<ItemAmount>();
				m_item.m_data = stream.Read<ItemDamage>();
			}
		}
	};

	// The clients active hotbar slot
	struct SetHotbarSlot : BasePacket {
		SetHotbarSlot() : BasePacket{ PacketId::SetHotbarSlot } {}
		NetworkSlotId m_slot;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_slot);
		}

		void Deserialize(NetworkStream& stream) override {
			m_slot = stream.Read<NetworkSlotId>();
		}
	};

	// Interactions with blocks
	struct InteractWithBlock : BasePacket {
		InteractWithBlock() : BasePacket{ PacketId::InteractWithBlock } {}
		EntityId m_entity_id;
		PacketData::BlockInteraction m_interaction_id;
		SlimInt3<int8_t> m_position;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_interaction_id);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_position.m_z);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_interaction_id = stream.Read<PacketData::BlockInteraction>();
			m_position.m_x = stream.Read<int32_t>();
			m_position.m_y = stream.Read<int8_t>();
			m_position.m_z = stream.Read<int32_t>();
		}
	};

	// Informs of the desired animation
	struct Animation : BasePacket {
		Animation() : BasePacket{ PacketId::Animation } {}
		EntityId m_entity_id;
		PacketData::Animation m_animation;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_animation);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_animation = stream.Read<PacketData::Animation>();
		}
	};

	// Used for simple actions, like sneaking
	struct PlayerAction : BasePacket {
		PlayerAction() : BasePacket{ PacketId::PlayerAction } {}
		EntityId m_entity_id;
		PacketData::PlayerAction m_action;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_action);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_action = stream.Read<PacketData::PlayerAction>();
		}
	};

	// Used for spawning other players in the world
	struct SpawnPlayer : BasePacket {
		SpawnPlayer() : BasePacket{ PacketId::SpawnPlayer } {}
		EntityId m_entity_id;
		std::string m_username;
		Int32_3 m_q_position;
		Int8_2 m_q_rotation;
		int8_t& m_q_yaw = m_q_rotation.m_x; // wire order: yaw first
		int8_t& m_q_pitch = m_q_rotation.m_y;
		ItemId m_held_item_id;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.WriteString16(m_username);
			stream.Write(m_q_position.m_x);
			stream.Write(m_q_position.m_y);
			stream.Write(m_q_position.m_z);
			stream.Write(m_q_yaw);
			stream.Write(m_q_pitch);
			stream.Write(m_held_item_id);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_username = stream.ReadString16();
			m_q_position.m_x = stream.Read<int32_t>();
			m_q_position.m_y = stream.Read<int32_t>();
			m_q_position.m_z = stream.Read<int32_t>();
			m_q_yaw = stream.Read<int8_t>();
			m_q_pitch = stream.Read<int8_t>();
			m_held_item_id = stream.Read<ItemId>();
		}
	};

	// Used for spawning items in the world
	struct SpawnItem : BasePacket {
		SpawnItem() : BasePacket{ PacketId::SpawnItem } {}
		EntityId m_entity_id;
		ItemStack m_item;
		Int32_3 m_q_position;
		Int8_3 m_q_rotation;
		int8_t& m_q_pitch = m_q_rotation.m_x;
		int8_t& m_q_yaw = m_q_rotation.m_y;
		int8_t& m_q_roll = m_q_rotation.m_z;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_item.m_id);
			stream.Write(m_item.m_count);
			stream.Write(m_item.m_data);
			stream.Write(m_q_position.m_x);
			stream.Write(m_q_position.m_y);
			stream.Write(m_q_position.m_z);
			stream.Write(m_q_pitch);
			stream.Write(m_q_yaw);
			stream.Write(m_q_roll);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_item.m_id = stream.Read<ItemId>();
			m_item.m_count = stream.Read<ItemAmount>();
			m_item.m_data = stream.Read<ItemDamage>();
			m_q_position.m_x = stream.Read<int32_t>();
			m_q_position.m_y = stream.Read<int32_t>();
			m_q_position.m_z = stream.Read<int32_t>();
			m_q_pitch = stream.Read<int8_t>();
			m_q_yaw = stream.Read<int8_t>();
			m_q_roll = stream.Read<int8_t>();
		}
	};

	// Used for collecting items visually
	struct CollectItem : BasePacket {
		CollectItem() : BasePacket{ PacketId::CollectItem } {}
		EntityId m_item_entity_id;
		EntityId m_collector_entity_id;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_item_entity_id);
			stream.Write(m_collector_entity_id);
		}

		void Deserialize(NetworkStream& stream) override {
			m_item_entity_id = stream.Read<EntityId>();
			m_collector_entity_id = stream.Read<EntityId>();
		}
	};

	// Used for spawning objects in the world
	struct SpawnObject : BasePacket {
		SpawnObject() : BasePacket{ PacketId::SpawnObject } {}
		EntityId m_entity_id;
		PacketData::ObjectType m_object_type;
		Int32_3 m_q_position;
		EntityId m_owner_entity_id = 0;
		Int16_3 m_q_velocity;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_object_type);
			stream.Write(m_q_position.m_x);
			stream.Write(m_q_position.m_y);
			stream.Write(m_q_position.m_z);
			stream.Write(m_owner_entity_id);
			if (m_owner_entity_id) {
				stream.Write(m_q_velocity.m_x);
				stream.Write(m_q_velocity.m_y);
				stream.Write(m_q_velocity.m_z);
			}
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_object_type = stream.Read<PacketData::ObjectType>();
			m_q_position.m_x = stream.Read<int32_t>();
			m_q_position.m_y = stream.Read<int32_t>();
			m_q_position.m_z = stream.Read<int32_t>();
			m_owner_entity_id = stream.Read<EntityId>();
			if (m_owner_entity_id) {
				m_q_velocity.m_x = stream.Read<int16_t>();
				m_q_velocity.m_y = stream.Read<int16_t>();
				m_q_velocity.m_z = stream.Read<int16_t>();
			}
		}
	};

	// Used for spawning mobs in the world
	struct SpawnMob : BasePacket {
		SpawnMob() : BasePacket{ PacketId::SpawnMob } {}
		EntityId m_entity_id;
		PacketData::MobType m_mob_type;
		Int32_3 m_q_position;
		Int8_2 m_q_rotation;
		int8_t& m_q_yaw = m_q_rotation.m_x; // wire order: yaw first
		int8_t& m_q_pitch = m_q_rotation.m_y;
		std::vector<PacketData::EntityMetadata::DataEntry> m_metadata;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_mob_type);
			stream.Write(m_q_position.m_x);
			stream.Write(m_q_position.m_y);
			stream.Write(m_q_position.m_z);
			stream.Write(m_q_yaw);
			stream.Write(m_q_pitch);
			stream.WriteEntityMetadata(m_metadata);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_mob_type = stream.Read<PacketData::MobType>();
			m_q_position.m_x = stream.Read<int32_t>();
			m_q_position.m_y = stream.Read<int32_t>();
			m_q_position.m_z = stream.Read<int32_t>();
			m_q_yaw = stream.Read<int8_t>();
			m_q_pitch = stream.Read<int8_t>();
			stream.ReadEntityMetadata(m_metadata);
		}
	};

	// Used for spawning paintings in the world
	struct SpawnPainting : BasePacket {
		SpawnPainting() : BasePacket{ PacketId::SpawnPainting } {}
		EntityId m_entity_id;
		std::string m_title;
		Int32_3 m_position; // Block position
		PacketData::PaintingDirection m_direction;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.WriteString16(m_title);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_position.m_z);
			stream.Write(m_direction);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_title = stream.ReadString16();
			m_position.m_x = stream.Read<int32_t>();
			m_position.m_y = stream.Read<int32_t>();
			m_position.m_z = stream.Read<int32_t>();
			m_direction = stream.Read<PacketData::PaintingDirection>();
		}
	};

	// Unused, but exists for sending raw player input to the client/server
	struct PlayerInput : BasePacket {
		PlayerInput() : BasePacket{ PacketId::PlayerInput } {}
		Float2 m_direction;
		float& m_strafe_direction = m_direction.m_x;
		float& m_forward_direction = m_direction.m_y;
		Float2 m_rotation;
		float& m_pitch = m_rotation.m_x;
		float& m_yaw = m_rotation.m_y;
		bool m_jumping;
		bool m_sneaking;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_strafe_direction);
			stream.Write(m_forward_direction);
			stream.Write(m_pitch);
			stream.Write(m_yaw);
			stream.Write(m_jumping);
			stream.Write(m_sneaking);
		}

		void Deserialize(NetworkStream& stream) override {
			m_strafe_direction = stream.Read<float>();
			m_forward_direction = stream.Read<float>();
			m_pitch = stream.Read<float>();
			m_yaw = stream.Read<float>();
			m_jumping = stream.Read<bool>();
			m_sneaking = stream.Read<bool>();
		}
	};

	// Used to update an entities velocity
	struct EntityVelocity : BasePacket {
		EntityVelocity() : BasePacket{ PacketId::EntityVelocity } {}
		EntityId m_entity_id;
		Int16_3 m_velocity;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_velocity.m_x);
			stream.Write(m_velocity.m_y);
			stream.Write(m_velocity.m_z);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_velocity.m_x = stream.Read<int16_t>();
			m_velocity.m_y = stream.Read<int16_t>();
			m_velocity.m_z = stream.Read<int16_t>();
		}
	};

	// Used to despawn an entity
	struct DespawnEntity : BasePacket {
		DespawnEntity() : BasePacket{ PacketId::DespawnEntity } {}
		EntityId m_entity_id;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
		}
	};

	// Unused, Base Packet for entity movement packets
	struct EntityMovement : BasePacket {
		EntityMovement() : BasePacket{ PacketId::EntityMovement } {}

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
		}

		void Deserialize([[maybe_unused]] NetworkStream& stream) override {}
	};

	// Used for setting an entitys relative position
	struct EntityPosition : BasePacket {
		EntityPosition() : BasePacket{ PacketId::EntityPosition } {}
		EntityId m_entity_id;
		Int8_3 m_qr_position;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_qr_position.m_x);
			stream.Write(m_qr_position.m_y);
			stream.Write(m_qr_position.m_z);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_qr_position.m_x = stream.Read<int8_t>();
			m_qr_position.m_y = stream.Read<int8_t>();
			m_qr_position.m_z = stream.Read<int8_t>();
		}
	};

	// Used for setting an entitys rotation
	struct EntityRotation : BasePacket {
		EntityRotation() : BasePacket{ PacketId::EntityRotation } {}
		EntityId m_entity_id;
		Int8_2 m_q_rotation;
		int8_t& m_q_yaw = m_q_rotation.m_x; // wire order: yaw first
		int8_t& m_q_pitch = m_q_rotation.m_y;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_q_yaw);
			stream.Write(m_q_pitch);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_q_yaw = stream.Read<int8_t>();
			m_q_pitch = stream.Read<int8_t>();
		}
	};

	// Used for setting an entitys relative position and rotation
	struct EntityPositionAndRotation : BasePacket {
		EntityPositionAndRotation() : BasePacket{ PacketId::EntityPositionAndRotation } {}
		EntityId m_entity_id;
		Int8_3 m_qr_position;
		Int8_2 m_q_rotation;
		int8_t& m_q_yaw = m_q_rotation.m_x; // wire order: yaw first
		int8_t& m_q_pitch = m_q_rotation.m_y;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_qr_position.m_x);
			stream.Write(m_qr_position.m_y);
			stream.Write(m_qr_position.m_z);
			stream.Write(m_q_yaw);
			stream.Write(m_q_pitch);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_qr_position.m_x = stream.Read<int8_t>();
			m_qr_position.m_y = stream.Read<int8_t>();
			m_qr_position.m_z = stream.Read<int8_t>();
			m_q_yaw = stream.Read<int8_t>();
			m_q_pitch = stream.Read<int8_t>();
		}
	};

	// Used for setting an entitys absolute position
	struct TeleportEntity : BasePacket {
		TeleportEntity() : BasePacket{ PacketId::TeleportEntity } {}
		EntityId m_entity_id;
		Int32_3 m_position;
		Int8_2 m_rotation;
		int8_t& m_yaw = m_rotation.m_x; // wire order: yaw first
		int8_t& m_pitch = m_rotation.m_y;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_position.m_z);
			stream.Write(m_yaw);
			stream.Write(m_pitch);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_position.m_x = stream.Read<int32_t>();
			m_position.m_y = stream.Read<int32_t>();
			m_position.m_z = stream.Read<int32_t>();
			m_yaw = stream.Read<int8_t>();
			m_pitch = stream.Read<int8_t>();
		}
	};

	// Used for some entity animations
	struct EntityEvent : BasePacket {
		EntityEvent() : BasePacket{ PacketId::EntityEvent } {}
		EntityId m_entity_id;
		PacketData::EntityEvent m_action;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_action);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_action = stream.Read<PacketData::EntityEvent>();
		}
	};

	// Used for mounting and dismounting entities
	struct AddPassenger : BasePacket {
		AddPassenger() : BasePacket{ PacketId::AddPassenger } {}
		EntityId m_passenger_entity_id;
		EntityId m_vehicle_entity_id;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_passenger_entity_id);
			stream.Write(m_vehicle_entity_id);
		}

		void Deserialize(NetworkStream& stream) override {
			m_passenger_entity_id = stream.Read<EntityId>();
			m_vehicle_entity_id = stream.Read<EntityId>();
		}
	};

	// Used for mounting and dismounting entities
	struct EntityMetadata : BasePacket {
		EntityMetadata() : BasePacket{ PacketId::EntityMetadata } {}
		EntityId m_entity_id;
		std::vector<PacketData::EntityMetadata::DataEntry> m_metadata;

		// TODO: Ideally this'd immediately read/write
		// the relevant data for the entity behind the ID,
		// but for now we'll just read it into the metadata vector

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.WriteEntityMetadata(m_metadata);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			stream.ReadEntityMetadata(m_metadata);
		}
	};

	// Tells the client to allocate or free a chunk slot. Must be sent before ChunkData
	struct SetChunkVisibility : BasePacket {
		SetChunkVisibility() : BasePacket{ PacketId::SetChunkVisibility } {}
		Int32_2 m_pos;
		bool m_visible;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_pos.m_x);
			stream.Write(m_pos.m_z);
			stream.Write(m_visible);
		}

		void Deserialize(NetworkStream& stream) override {
			m_pos.m_x = stream.Read<int32_t>();
			m_pos.m_z = stream.Read<int32_t>();
			m_visible = stream.Read<bool>();
		}
	};

	// Sends compressed chunk data; always preceded by SetChunkVisibility
	struct ChunkData : BasePacket {
		ChunkData() : BasePacket{ PacketId::Chunk } {}
		SlimInt3<int16_t> m_pos{ 0, 0, 0 };
		TriNumber<uint8_t> m_size{ CHUNK_WIDTH - 1, CHUNK_HEIGHT - 1, CHUNK_WIDTH - 1 };
		std::vector<uint8_t> m_compressedData;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_pos.m_x);
			stream.Write(m_pos.m_y);
			stream.Write(m_pos.m_z);
			stream.Write(m_size.m_x);
			stream.Write(m_size.m_y);
			stream.Write(m_size.m_z);
			stream.Write(static_cast<int32_t>(m_compressedData.size()));
			stream.WriteBytes(m_compressedData.data(), m_compressedData.size());
		}

		void Deserialize(NetworkStream& stream) override {
			m_pos.m_x = stream.Read<int32_t>();
			m_pos.m_y = stream.Read<int16_t>();
			m_pos.m_z = stream.Read<int32_t>();
			m_size.m_x = stream.Read<uint8_t>();
			m_size.m_y = stream.Read<uint8_t>();
			m_size.m_z = stream.Read<uint8_t>();
			int32_t length = stream.Read<int32_t>();
			m_compressedData.resize(static_cast<size_t>(length));
			stream.ReadBytes(m_compressedData.data(), static_cast<size_t>(length));
		}
	};

	// Used to set multiple blocks in a small area
	struct SetMultipleBlocks : BasePacket {
		SetMultipleBlocks() : BasePacket{ PacketId::SetMultipleBlocks } {}
		Int32_2 m_chunk_position;
		int16_t m_number_of_blocks;
		std::vector<int16_t> m_block_coordinates;
		std::vector<BlockType> m_block_types;
		std::vector<int8_t> m_block_metadata; // Nibbles

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_chunk_position.m_x);
			stream.Write(m_chunk_position.m_z);
			stream.Write(m_number_of_blocks);
			for (int16_t i = 0; i < m_number_of_blocks; i++)
				stream.Write(m_block_coordinates[static_cast<size_t>(i)]);
			for (int16_t i = 0; i < m_number_of_blocks; i++)
				stream.Write(m_block_types[static_cast<size_t>(i)]);
			for (int16_t i = 0; i < m_number_of_blocks; i++)
				stream.Write(m_block_metadata[static_cast<size_t>(i)]);
		}

		void Deserialize(NetworkStream& stream) override {
			m_chunk_position.m_x = stream.Read<int32_t>();
			m_chunk_position.m_z = stream.Read<int32_t>();
			m_number_of_blocks = stream.Read<int16_t>();
			m_block_coordinates.resize(static_cast<size_t>(m_number_of_blocks));
			m_block_types.resize(static_cast<size_t>(m_number_of_blocks));
			m_block_metadata.resize(static_cast<size_t>(m_number_of_blocks));
			for (int16_t i = 0; i < m_number_of_blocks; i++)
				m_block_coordinates[static_cast<size_t>(i)] = stream.Read<int16_t>();
			for (int16_t i = 0; i < m_number_of_blocks; i++)
				m_block_types[static_cast<size_t>(i)] = stream.Read<BlockType>();
			for (int16_t i = 0; i < m_number_of_blocks; i++)
				m_block_metadata[static_cast<size_t>(i)] = stream.Read<int8_t>();
		}
	};

	// Used to set a singular block
	struct SetBlock : BasePacket {
		SetBlock() : BasePacket{ PacketId::SetBlock } {}
		SlimInt3<int8_t> m_position;
		Block m_block;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_position.m_z);
			stream.Write(m_block.m_type);
			stream.Write(m_block.m_data);
		}

		void Deserialize(NetworkStream& stream) override {
			m_position.m_x = stream.Read<int32_t>();
			m_position.m_y = stream.Read<int8_t>();
			m_position.m_z = stream.Read<int32_t>();
			m_block.m_type = stream.Read<BlockType>();
			m_block.m_data = stream.Read<uint8_t>();
		}
	};

	// Used to set a singular block
	struct BlockEvent : BasePacket {
		BlockEvent() : BasePacket{ PacketId::BlockEvent } {}
		SlimInt3<int8_t> m_position;
		int8_t m_instrument_state;
		int8_t m_pitch_direction;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_position.m_z);
			stream.Write(m_instrument_state);
			stream.Write(m_pitch_direction);
		}

		void Deserialize(NetworkStream& stream) override {
			m_position.m_x = stream.Read<int32_t>();
			m_position.m_y = stream.Read<int8_t>();
			m_position.m_z = stream.Read<int32_t>();
			m_instrument_state = stream.Read<int8_t>();
			m_pitch_direction = stream.Read<int8_t>();
		}

		PacketData::NoteInstrument instrument() const {
			return static_cast<PacketData::NoteInstrument>(m_instrument_state);
		}

		PacketData::NotePitch pitch() const {
			return static_cast<PacketData::NotePitch>(m_pitch_direction);
		}

		PacketData::PistonState state() const {
			return static_cast<PacketData::PistonState>(m_instrument_state);
		}

		PacketData::PistonDirection direction() const {
			return static_cast<PacketData::PistonDirection>(m_pitch_direction);
		}
	};

	// Used for explosions
	struct Explosion : BasePacket {
		Explosion() : BasePacket{ PacketId::Explosion } {}
		Vec3 m_position;
		float m_radius;
		int32_t m_number_of_destroyed_blocks;
		std::vector<int8_t> m_destroyed_blocks;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_position.m_z);
			stream.Write(m_radius);
			stream.Write(static_cast<int32_t>(m_destroyed_blocks.size()));
			stream.WriteBytes(reinterpret_cast<const uint8_t*>(m_destroyed_blocks.data()), m_destroyed_blocks.size());
		}

		void Deserialize(NetworkStream& stream) override {
			m_position.m_x = stream.Read<double>();
			m_position.m_y = stream.Read<double>();
			m_position.m_z = stream.Read<double>();
			m_radius = stream.Read<float>();
			m_number_of_destroyed_blocks = stream.Read<int32_t>();
			m_destroyed_blocks.resize(static_cast<size_t>(m_number_of_destroyed_blocks));
			stream.ReadBytes(reinterpret_cast<uint8_t*>(m_destroyed_blocks.data()),
			                 static_cast<size_t>(m_number_of_destroyed_blocks));
		}
	};

	// Used to trigger world events, such as sound effects
	struct WorldEvent : BasePacket {
		WorldEvent() : BasePacket{ PacketId::WorldEvent } {}
		PacketData::WorldEvent m_event_id;
		SlimInt3<int8_t> m_position;
		int32_t m_data;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_z);
			stream.Write(m_position.m_y);
			stream.Write(m_event_id);
			stream.Write(m_data);
		}

		void Deserialize(NetworkStream& stream) override {
			m_position.m_x = stream.Read<int32_t>();
			m_position.m_z = stream.Read<int32_t>();
			m_position.m_y = stream.Read<int8_t>();
			m_event_id = stream.Read<PacketData::WorldEvent>();
			m_data = stream.Read<int32_t>();
		}
	};

	// Used to trigger global game events, such as rain
	struct GameEvent : BasePacket {
		GameEvent() : BasePacket{ PacketId::GameEvent } {}
		PacketData::GameEvent m_event_id;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_event_id);
		}

		void Deserialize(NetworkStream& stream) override {
			m_event_id = stream.Read<PacketData::GameEvent>();
		}
	};

	// Used to spawn a lightning bolt
	struct LightningBolt : BasePacket {
		LightningBolt() : BasePacket{ PacketId::LightningBolt } {}
		EntityId m_entity_id;
		// This is only ever "1", which means lightning
		int8_t m_entity_type = 1;
		Int32_3 m_position;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_entity_id);
			stream.Write(m_entity_type);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_position.m_z);
		}

		void Deserialize(NetworkStream& stream) override {
			m_entity_id = stream.Read<EntityId>();
			m_entity_type = stream.Read<int8_t>();
			m_position.m_x = stream.Read<int32_t>();
			m_position.m_y = stream.Read<int32_t>();
			m_position.m_z = stream.Read<int32_t>();
		}
	};

	// Used for signaling when a container is opened
	struct OpenContainer : BasePacket {
		OpenContainer() : BasePacket{ PacketId::OpenContainer } {}
		WindowId m_window_id;
		PacketData::WindowType m_window_type;
		std::string m_title; // This is String8!!
		int8_t m_slot_count;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_window_id);
			stream.Write(m_window_type);
			stream.WriteString8(m_title);
			stream.Write(m_slot_count);
		}
		void Deserialize(NetworkStream& stream) override {
			m_window_id = stream.Read<WindowId>();
			m_window_type = stream.Read<PacketData::WindowType>();
			m_title = stream.ReadString8();
			m_slot_count = stream.Read<int8_t>();
		}
	};

	// Used for signaling when a container was closed
	struct CloseContainer : BasePacket {
		CloseContainer() : BasePacket{ PacketId::CloseContainer } {}
		WindowId m_window_id;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_window_id);
		}
		void Deserialize(NetworkStream& stream) override {
			m_window_id = stream.Read<WindowId>();
		}
	};

	// Used for signaling when a slot was clicked
	struct ClickSlot : BasePacket {
		ClickSlot() : BasePacket{ PacketId::ClickSlot } {}
		WindowId m_window_id;
		NetworkSlotId m_slot_id;
		bool m_right_click;
		TransactionId m_transaction_id;
		bool m_shift;
		ItemStack m_item;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_window_id);
			stream.Write(m_slot_id);
			stream.Write(m_right_click);
			stream.Write(m_transaction_id);
			stream.Write(m_shift);
			stream.Write(m_item.m_id);
			if (m_item.m_id != Items::Id::INVALID) {
				stream.Write(m_item.m_count);
				stream.Write(m_item.m_data);
			}
		}
		void Deserialize(NetworkStream& stream) override {
			m_window_id = stream.Read<WindowId>();
			m_slot_id = stream.Read<NetworkSlotId>();
			m_right_click = stream.Read<bool>();
			m_transaction_id = stream.Read<TransactionId>();
			m_shift = stream.Read<bool>();
			m_item.m_id = stream.Read<ItemId>();
			if (m_item.m_id != Items::Id::INVALID) {
				m_item.m_count = stream.Read<ItemAmount>();
				m_item.m_data = stream.Read<ItemDamage>();
			}
		}
	};

	// Used for setting the contents of a slot
	struct SetSlot : BasePacket {
		SetSlot() : BasePacket{ PacketId::SetSlot } {}
		WindowId m_window_id;
		NetworkSlotId m_slot_id;
		ItemStack m_item;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_window_id);
			stream.Write(m_slot_id);
			stream.Write(m_item.m_id);
			if (m_item.m_id != Items::Id::INVALID) {
				stream.Write(m_item.m_count);
				stream.Write(m_item.m_data);
			}
		}
		void Deserialize(NetworkStream& stream) override {
			m_window_id = stream.Read<WindowId>();
			m_slot_id = stream.Read<NetworkSlotId>();
			m_item.m_id = stream.Read<ItemId>();
			if (m_item.m_id != Items::Id::INVALID) {
				m_item.m_count = stream.Read<ItemAmount>();
				m_item.m_data = stream.Read<ItemDamage>();
			}
		}
	};

	// Possibly we do this by passing in the whole inventory?
	// Used for filling a container with data
	struct FillContainer : BasePacket {
		FillContainer() : BasePacket{ PacketId::FillContainer } {}
		WindowId m_window_id;
		std::vector<ItemStack> m_items;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_window_id);
			stream.Write(int16_t(m_items.size()));
			for (ItemStack item : m_items) {
				stream.Write(item.m_id);
				if (item.m_id != Items::Id::INVALID) {
					stream.Write(item.m_count);
					stream.Write(item.m_data);
				}
			}
		}
		void Deserialize(NetworkStream& stream) override {
			m_window_id = stream.Read<WindowId>();
			size_t number_of_slots = size_t(stream.Read<int16_t>());
			m_items.resize(number_of_slots, ItemStack{ Items::Id::INVALID });
			for (size_t i = 0; i < number_of_slots; i++) {
				m_items[i].m_id = stream.Read<ItemId>();
				if (m_items[i].m_id != Items::Id::INVALID) {
					m_items[i].m_count = stream.Read<ItemAmount>();
					m_items[i].m_data = stream.Read<ItemDamage>();
				}
			}
		}
	};

	// Used for setting data for containers, such as furnace progress
	struct ContainerData : BasePacket {
		ContainerData() : BasePacket{ PacketId::ContainerData } {}
		WindowId m_window_id;
		struct {
			PacketData::ContainerDataType m_type;
			int16_t m_value;
		} m_container_data;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_window_id);
			stream.Write(m_container_data.m_type);
			stream.Write(m_container_data.m_value);
		}
		void Deserialize(NetworkStream& stream) override {
			m_window_id = stream.Read<WindowId>();
			m_container_data.m_type = stream.Read<PacketData::ContainerDataType>();
			m_container_data.m_value = stream.Read<int16_t>();
		}
	};

	// Used for checking if the performed transaction was valid and got through successfully
	struct ContainerTransaction : BasePacket {
		ContainerTransaction() : BasePacket{ PacketId::ContainerTransaction } {}
		WindowId m_window_id;
		TransactionId m_transaction_id;
		bool m_accepted;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_window_id);
			stream.Write(m_transaction_id);
			stream.Write(m_accepted);
		}
		void Deserialize(NetworkStream& stream) override {
			m_window_id = stream.Read<WindowId>();
			m_transaction_id = stream.Read<TransactionId>();
			m_accepted = stream.Read<bool>();
		}
	};

	// Use for updating the text on signs
	struct UpdateSign : BasePacket {
		UpdateSign() : BasePacket{ PacketId::UpdateSign } {}
		SlimInt3<int16_t> m_position;
		std::string m_lines[4];

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_position.m_x);
			stream.Write(m_position.m_y);
			stream.Write(m_position.m_z);
			stream.WriteString16(m_lines[0]);
			stream.WriteString16(m_lines[1]);
			stream.WriteString16(m_lines[2]);
			stream.WriteString16(m_lines[3]);
		}

		void Deserialize(NetworkStream& stream) override {
			m_position.m_x = stream.Read<int32_t>();
			m_position.m_y = stream.Read<int16_t>();
			m_position.m_z = stream.Read<int32_t>();
			m_lines[0] = stream.ReadString16();
			m_lines[1] = stream.ReadString16();
			m_lines[2] = stream.ReadString16();
			m_lines[3] = stream.ReadString16();
		}
	};

	// Used for updating custom item data, only used by maps
	struct ItemData : BasePacket {
		ItemData() : BasePacket{ PacketId::ItemData } {}
		ItemId m_item_id;
		MapId m_map_id;
		std::vector<uint8_t> m_data;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_item_id);
			stream.Write(m_map_id);
			stream.Write(uint8_t(m_data.size()));
			stream.WriteBytes(m_data.data(), m_data.size());
		}

		void Deserialize(NetworkStream& stream) override {
			m_item_id = stream.Read<ItemId>();
			m_map_id = stream.Read<MapId>();
			uint8_t size = stream.Read<uint8_t>();
			m_data.resize(size_t(size));
			stream.ReadBytes(m_data.data(), size_t(size));
		}
	};

	// Used for changing the value of a statistic
	struct IncrementStatistic : BasePacket {
		IncrementStatistic() : BasePacket{ PacketId::IncrementStatistic } {}
		int32_t m_statistic_id; // TODO: Replace with Enum
		int8_t m_amount;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.Write(m_statistic_id);
			stream.Write(m_amount);
		}

		void Deserialize(NetworkStream& stream) override {
			m_statistic_id = stream.Read<int32_t>();
			m_amount = stream.Read<int8_t>();
		}
	};

	// Used for disconnecting with a disconnect reason
	struct Disconnect : BasePacket {
		Disconnect() : BasePacket{ PacketId::Disconnect } {}
		std::string m_reason;

		void Serialize(NetworkStream& stream) const override {
			stream.Write(m_id);
			stream.WriteString16(m_reason);
		}

		void Deserialize(NetworkStream& stream) override {
			m_reason = stream.ReadString16();
		}
	};
};