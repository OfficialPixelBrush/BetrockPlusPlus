/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "../entities/entity_mp_player.h"
#include "../entities/entity_tracker.h"
#include "inventory/interactions/player.h"
#include "inventory/inventory_interaction.h"
#include "items.h"
#include "nbt/nbt.h"
#include "networking/network_stream.h"
#include "world/client_pos.h"
#include "world/world.h"
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class ConnectionState : uint8_t {
	Handshaking,
	LoggingIn,
	WaitingForSpawnChunks,
	Playing
};

struct PlayerSession {
	NetworkStream m_stream;
	ClientPosition m_position;

	// What our client is claiming this tick
	std::optional<Vec3> m_pendingPosition = std::nullopt;
	std::optional<Vec3> m_pendingTeleport = std::nullopt;

	// Our player entity
	std::shared_ptr<EntityMPPlayer> m_entity;
	EntityTracker* m_entityTracker = nullptr;
	bool m_entityRegistered = false;

	// rotation.x = yaw, rotation.y = pitch
	Float2 m_rotation = { 0.0f, 0.0f };

	std::unordered_set<Int32_2> m_sentChunks;
	std::unordered_set<Int32_2> m_flushedChunks; // Actually written to stream

	// Chunks that were written to the stream during the last flush() call.
	std::vector<Int32_2> m_newlyFlushed;

	// Chunks that were unloaded during the last enqueue() call.
	std::vector<Int32_2> m_newlyUnloaded;

	// Block updates that arrived while the chunk was enqueued but not yet flushed.
	std::unordered_map<Int32_2, std::vector<PendingBlock>> m_pendingBlockChanges;

	ConnectionState m_connState = ConnectionState::Handshaking;
	std::string m_username;
	std::chrono::steady_clock::time_point m_last_packet_time = std::chrono::steady_clock::now();

	// Inventory
	InventoryPlayer m_inventory;
	PlayerInventoryInteraction m_inventoryInteraction;
	std::unique_ptr<InventoryInteraction> m_activeInteraction = nullptr;

	// windowId = 0 is always the player inventory. Non-zero means a container is open.
	// ranges from 0-127 and wraps
	WindowId m_openWindowId = 0;
	WindowId getNextWindowId() {
		m_openWindowId = (m_openWindowId + 1) % 128;
		return m_openWindowId;
	}

	// Lock after a rejected click until client acknowledges the resync.
	// While locked, all incoming clicks are rejected to prevent state corruption.
	bool m_inventoryLocked = false;
	TransactionId m_pendingTransactionId = 0;
	WindowId m_pendingWindowId = 0;

	int8_t m_dimension = 0; // 0 = overworld, -1 = nether

	BlockType m_lastTargetedBlock = BLOCK_AIR;
	TickTime m_startedMiningAtTick = 0;

	explicit PlayerSession(int socket, Runtime& gameRuntime)
	    : m_stream(socket), m_inventoryInteraction(&m_inventory, gameRuntime) {}
	~PlayerSession() {
		// So our player entity despawns from the world
		if (m_entity) {
			m_entity->m_isDead = true;
			m_entity->m_session = nullptr;
		}
		m_entityTracker = nullptr;
	}

	// Load our player data from file
	void loadPlayerNBT(Tag& nbt) {
		// Very basic but just stuff we care about for now
		auto& it = nbt.get("Pos").getList();
		m_position.m_pos.m_x = it[0].getDouble();
		m_position.m_pos.m_y = it[1].getDouble();
		m_position.m_pos.m_z = it[2].getDouble();

		auto& it2 = nbt.get("Rotation").getList();
		m_rotation.m_x = it2[0].getFloat();
		m_rotation.m_y = it2[1].getFloat();

		m_dimension = static_cast<int8_t>(nbt.get("Dimension").getInt());

		auto& it3 = nbt.get("Inventory").getList();
		for (auto& item : it3) {
			NbtSlotId nbtSlot = item.get("Slot").getByte();
			NetworkSlotId networkSlot = m_inventory.getNetworkSlotId(nbtSlot);
			if (networkSlot < 0 || networkSlot >= int(m_inventory.m_slots.size()))
				continue;
			m_inventory.m_slots[size_t(networkSlot)] = ItemStack{ item.get("id").getShort(), item.get("Count").getByte(),
				                                              item.get("Damage").getShort() };
		}
	}

	Tag serializeToNBT() {
		Tag root;
		root.m_type = TAG_COMPOUND;
		root.m_name = "";

		Tag Motion;
		Motion.m_type = TAG_LIST;
		Motion.m_name = "Motion";
		Motion.m_listType = TAG_DOUBLE;
		Tag SleepTimer;
		SleepTimer.m_type = TAG_SHORT;
		SleepTimer.m_name = "SleepTimer";
		SleepTimer.m_shortValue = 0;
		Tag Health;
		Health.m_type = TAG_SHORT;
		Health.m_name = "Health";
		Health.m_shortValue = 20;
		Tag Air;
		Air.m_type = TAG_SHORT;
		Air.m_name = "Air";
		Air.m_shortValue = 300;
		Tag OnGround;
		OnGround.m_type = TAG_BYTE;
		OnGround.m_name = "OnGround";
		OnGround.m_byteValue = 0;
		Tag Dimension;
		Dimension.m_type = TAG_INT;
		Dimension.m_name = "Dimension";
		Dimension.m_intValue = m_dimension;
		Tag Rotation;
		Rotation.m_type = TAG_LIST;
		Rotation.m_name = "Rotation";
		Rotation.m_listType = TAG_FLOAT;
		Tag FallDistance;
		FallDistance.m_type = TAG_FLOAT;
		FallDistance.m_name = "FallDistance";
		FallDistance.m_floatValue = 0.0f;
		Tag Sleeping;
		Sleeping.m_type = TAG_BYTE;
		Sleeping.m_name = "Sleeping";
		Sleeping.m_byteValue = 0;
		Tag Pos;
		Pos.m_type = TAG_LIST;
		Pos.m_name = "Pos";
		Pos.m_listType = TAG_DOUBLE;
		Tag DeathTime;
		DeathTime.m_type = TAG_SHORT;
		DeathTime.m_name = "DeathTime";
		DeathTime.m_shortValue = 0;
		Tag Fire;
		Fire.m_type = TAG_SHORT;
		Fire.m_name = "Fire";
		Fire.m_shortValue = -20;
		Tag HurtTime;
		HurtTime.m_type = TAG_SHORT;
		HurtTime.m_name = "HurtTime";
		HurtTime.m_shortValue = 0;
		Tag AttackTime;
		AttackTime.m_type = TAG_SHORT;
		AttackTime.m_name = "AttackTime";
		AttackTime.m_shortValue = 0;
		Tag Inventory;
		Inventory.m_type = TAG_LIST;
		Inventory.m_name = "Inventory";
		Inventory.m_listType = TAG_COMPOUND;

		// Save position and rotation
		Tag posX;
		posX.m_type = TAG_DOUBLE;
		posX.m_doubleValue = m_position.m_pos.m_x;
		Tag posY;
		posY.m_type = TAG_DOUBLE;
		posY.m_doubleValue = m_position.m_pos.m_y;
		Tag posZ;
		posZ.m_type = TAG_DOUBLE;
		posZ.m_doubleValue = m_position.m_pos.m_z;
		Pos.m_list.push_back(posX);
		Pos.m_list.push_back(posY);
		Pos.m_list.push_back(posZ);

		Tag rotX;
		rotX.m_type = TAG_FLOAT;
		rotX.m_floatValue = m_rotation.m_x;
		Tag rotY;
		rotY.m_type = TAG_FLOAT;
		rotY.m_floatValue = m_rotation.m_y;
		Rotation.m_list.push_back(rotX);
		Rotation.m_list.push_back(rotY);

		// Initialize our position with a default
		Tag movX;
		movX.m_type = TAG_DOUBLE;
		movX.m_doubleValue = 0.0;
		Tag movY;
		movY.m_type = TAG_DOUBLE;
		movY.m_doubleValue = 0.0;
		Tag movZ;
		movZ.m_type = TAG_DOUBLE;
		movZ.m_doubleValue = 0.0;
		Motion.m_list.push_back(movX);
		Motion.m_list.push_back(movY);
		Motion.m_list.push_back(movZ);

		// Save our current inventory
		NetworkSlotId slotId = 0;
		for (auto& item : m_inventory.m_slots) {
			if (item.m_id != Items::Id::INVALID) {
				Tag itemTag;
				itemTag.m_type = TAG_COMPOUND;
				itemTag.m_name = "";
				Tag slotTag;
				slotTag.m_type = TAG_BYTE;
				slotTag.m_name = "Slot";
				slotTag.m_byteValue = m_inventory.getNbtSlotID(slotId);
				Tag idTag;
				idTag.m_type = TAG_SHORT;
				idTag.m_name = "id";
				idTag.m_shortValue = item.m_id;
				Tag countTag;
				countTag.m_type = TAG_BYTE;
				countTag.m_name = "Count";
				countTag.m_byteValue = item.m_count;
				Tag damageTag;
				damageTag.m_type = TAG_SHORT;
				damageTag.m_name = "Damage";
				damageTag.m_shortValue = item.m_data;

				itemTag.m_compound["Slot"] = slotTag;
				itemTag.m_compound["id"] = idTag;
				itemTag.m_compound["Count"] = countTag;
				itemTag.m_compound["Damage"] = damageTag;
				Inventory.m_list.push_back(itemTag);
			}
			slotId++;
		}

		root.m_compound["Motion"] = Motion;
		root.m_compound["SleepTimer"] = SleepTimer;
		root.m_compound["Health"] = Health;
		root.m_compound["Air"] = Air;
		root.m_compound["OnGround"] = OnGround;
		root.m_compound["Dimension"] = Dimension;
		root.m_compound["Rotation"] = Rotation;
		root.m_compound["FallDistance"] = FallDistance;
		root.m_compound["Sleeping"] = Sleeping;
		root.m_compound["Pos"] = Pos;
		root.m_compound["DeathTime"] = DeathTime;
		root.m_compound["Fire"] = Fire;
		root.m_compound["HurtTime"] = HurtTime;
		root.m_compound["AttackTime"] = AttackTime;
		root.m_compound["Inventory"] = Inventory;

		return root;
	}
};