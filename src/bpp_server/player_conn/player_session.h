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
	NetworkStream stream;
	ClientPosition position;

	// What our client is claiming this tick
	std::optional<Vec3> pendingPosition = std::nullopt;
	std::optional<Vec3> pendingTeleport = std::nullopt;

	// Our player entity
	std::shared_ptr<EntityMPPlayer> entity;
	EntityTracker* entityTracker = nullptr;
	bool entityRegistered = false;

	// rotation.x = yaw, rotation.y = pitch
	Float2 rotation = { 0.0f, 0.0f };

	std::unordered_set<Int32_2> sentChunks;
	std::unordered_set<Int32_2> flushedChunks; // Actually written to stream

	// Chunks that were written to the stream during the last flush() call.
	std::vector<Int32_2> newlyFlushed;

	// Chunks that were unloaded during the last enqueue() call.
	std::vector<Int32_2> newlyUnloaded;

	// Block updates that arrived while the chunk was enqueued but not yet flushed.
	std::unordered_map<Int32_2, std::vector<PendingBlock>> pendingBlockChanges;

	ConnectionState connState = ConnectionState::Handshaking;
	std::string username;
	std::chrono::steady_clock::time_point last_packet_time = std::chrono::steady_clock::now();

	// Inventory
	InventoryPlayer inventory;
	PlayerInventoryInteraction inventoryInteraction;
	std::unique_ptr<InventoryInteraction> activeInteraction = nullptr;

	// windowId = 0 is always the player inventory. Non-zero means a container is open.
	// ranges from 0-127 and wraps
	WindowId openWindowId = 0;
	WindowId getNextWindowId() {
		openWindowId = (openWindowId + 1) % 128;
		return openWindowId;
	}

	// Lock after a rejected click until client acknowledges the resync.
	// While locked, all incoming clicks are rejected to prevent state corruption.
	bool inventoryLocked = false;
	TransactionId pendingTransactionId = 0;
	WindowId pendingWindowId = 0;

	int8_t dimension = 0; // 0 = overworld, -1 = nether

	BlockType lastTargetedBlock = BLOCK_AIR;
	TickTime startedMiningAtTick = 0;

	explicit PlayerSession(int socket, Runtime& gameRuntime)
	    : stream(socket), inventoryInteraction(&inventory, gameRuntime) {}
	~PlayerSession() {
		// So our player entity despawns from the world
		if (entity) {
			entity->isDead = true;
			entity->session = nullptr;
		}
		entityTracker = nullptr;
	}

	// Load our player data from file
	void loadPlayerNBT(Tag& nbt) {
		// Very basic but just stuff we care about for now
		auto& it = nbt.get("Pos").getList();
		position.pos.x = it[0].getDouble();
		position.pos.y = it[1].getDouble();
		position.pos.z = it[2].getDouble();

		auto& it2 = nbt.get("Rotation").getList();
		rotation.x = it2[0].getFloat();
		rotation.y = it2[1].getFloat();

		dimension = static_cast<int8_t>(nbt.get("Dimension").getInt());

		auto& it3 = nbt.get("Inventory").getList();
		for (auto& item : it3) {
			NbtSlotId nbtSlot = item.get("Slot").getByte();
			NetworkSlotId networkSlot = inventory.getNetworkSlotId(nbtSlot);
			if (networkSlot < 0 || networkSlot >= int(inventory.slots.size()))
				continue;
			inventory.slots[size_t(networkSlot)] = ItemStack{ item.get("id").getShort(), item.get("Count").getByte(),
				                                              item.get("Damage").getShort() };
		}
	}

	Tag serializeToNBT() {
		Tag root;
		root.type = TAG_COMPOUND;
		root.name = "";

		Tag Motion;
		Motion.type = TAG_LIST;
		Motion.name = "Motion";
		Motion.listType = TAG_DOUBLE;
		Tag SleepTimer;
		SleepTimer.type = TAG_SHORT;
		SleepTimer.name = "SleepTimer";
		SleepTimer.shortValue = 0;
		Tag Health;
		Health.type = TAG_SHORT;
		Health.name = "Health";
		Health.shortValue = 20;
		Tag Air;
		Air.type = TAG_SHORT;
		Air.name = "Air";
		Air.shortValue = 300;
		Tag OnGround;
		OnGround.type = TAG_BYTE;
		OnGround.name = "OnGround";
		OnGround.byteValue = 0;
		Tag Dimension;
		Dimension.type = TAG_INT;
		Dimension.name = "Dimension";
		Dimension.intValue = dimension;
		Tag Rotation;
		Rotation.type = TAG_LIST;
		Rotation.name = "Rotation";
		Rotation.listType = TAG_FLOAT;
		Tag FallDistance;
		FallDistance.type = TAG_FLOAT;
		FallDistance.name = "FallDistance";
		FallDistance.floatValue = 0.0f;
		Tag Sleeping;
		Sleeping.type = TAG_BYTE;
		Sleeping.name = "Sleeping";
		Sleeping.byteValue = 0;
		Tag Pos;
		Pos.type = TAG_LIST;
		Pos.name = "Pos";
		Pos.listType = TAG_DOUBLE;
		Tag DeathTime;
		DeathTime.type = TAG_SHORT;
		DeathTime.name = "DeathTime";
		DeathTime.shortValue = 0;
		Tag Fire;
		Fire.type = TAG_SHORT;
		Fire.name = "Fire";
		Fire.shortValue = -20;
		Tag HurtTime;
		HurtTime.type = TAG_SHORT;
		HurtTime.name = "HurtTime";
		HurtTime.shortValue = 0;
		Tag AttackTime;
		AttackTime.type = TAG_SHORT;
		AttackTime.name = "AttackTime";
		AttackTime.shortValue = 0;
		Tag Inventory;
		Inventory.type = TAG_LIST;
		Inventory.name = "Inventory";
		Inventory.listType = TAG_COMPOUND;

		// Save position and rotation
		Tag posX;
		posX.type = TAG_DOUBLE;
		posX.doubleValue = position.pos.x;
		Tag posY;
		posY.type = TAG_DOUBLE;
		posY.doubleValue = position.pos.y;
		Tag posZ;
		posZ.type = TAG_DOUBLE;
		posZ.doubleValue = position.pos.z;
		Pos.list.push_back(posX);
		Pos.list.push_back(posY);
		Pos.list.push_back(posZ);

		Tag rotX;
		rotX.type = TAG_FLOAT;
		rotX.floatValue = rotation.x;
		Tag rotY;
		rotY.type = TAG_FLOAT;
		rotY.floatValue = rotation.y;
		Rotation.list.push_back(rotX);
		Rotation.list.push_back(rotY);

		// Initialize our position with a default
		Tag movX;
		movX.type = TAG_DOUBLE;
		movX.doubleValue = 0.0;
		Tag movY;
		movY.type = TAG_DOUBLE;
		movY.doubleValue = 0.0;
		Tag movZ;
		movZ.type = TAG_DOUBLE;
		movZ.doubleValue = 0.0;
		Motion.list.push_back(movX);
		Motion.list.push_back(movY);
		Motion.list.push_back(movZ);

		// Save our current inventory
		NetworkSlotId slotId = 0;
		for (auto& item : inventory.slots) {
			if (item.id != Items::Id::INVALID) {
				Tag itemTag;
				itemTag.type = TAG_COMPOUND;
				itemTag.name = "";
				Tag slotTag;
				slotTag.type = TAG_BYTE;
				slotTag.name = "Slot";
				slotTag.byteValue = inventory.getNbtSlotID(slotId);
				Tag idTag;
				idTag.type = TAG_SHORT;
				idTag.name = "id";
				idTag.shortValue = item.id;
				Tag countTag;
				countTag.type = TAG_BYTE;
				countTag.name = "Count";
				countTag.byteValue = item.count;
				Tag damageTag;
				damageTag.type = TAG_SHORT;
				damageTag.name = "Damage";
				damageTag.shortValue = item.data;

				itemTag.compound["Slot"] = slotTag;
				itemTag.compound["id"] = idTag;
				itemTag.compound["Count"] = countTag;
				itemTag.compound["Damage"] = damageTag;
				Inventory.list.push_back(itemTag);
			}
			slotId++;
		}

		root.compound["Motion"] = Motion;
		root.compound["SleepTimer"] = SleepTimer;
		root.compound["Health"] = Health;
		root.compound["Air"] = Air;
		root.compound["OnGround"] = OnGround;
		root.compound["Dimension"] = Dimension;
		root.compound["Rotation"] = Rotation;
		root.compound["FallDistance"] = FallDistance;
		root.compound["Sleeping"] = Sleeping;
		root.compound["Pos"] = Pos;
		root.compound["DeathTime"] = DeathTime;
		root.compound["Fire"] = Fire;
		root.compound["HurtTime"] = HurtTime;
		root.compound["AttackTime"] = AttackTime;
		root.compound["Inventory"] = Inventory;

		return root;
	}
};