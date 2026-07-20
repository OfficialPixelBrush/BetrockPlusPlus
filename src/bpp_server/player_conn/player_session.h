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

	// What our client is claiming this Tick
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
	std::chrono::steady_clock::time_point lastPacketTime = std::chrono::steady_clock::now();

	// Inventory
	InventoryPlayer inventory;
	PlayerInventoryInteraction inventoryInteraction;
	std::unique_ptr<InventoryInteraction> activeInteraction = nullptr;

	// windowId = 0 is always the player inventory. Non-zero means a container is open.
	// ranges from 0-127 and wraps
	WindowId openWindowId = 0;
	WindowId GetNextWindowId() {
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

	explicit PlayerSession(int _socket, Runtime& _gameRuntime)
	    : stream(_socket), inventoryInteraction(&inventory, _gameRuntime) {}
	~PlayerSession() {
		// So our player entity despawns from the world
		if (entity) {
			entity->isDead = true;
			entity->session = nullptr;
		}
		entityTracker = nullptr;
	}

	// Load our player data from file
	void LoadPlayerNbt(Tag& _nbt) {
		// Very basic but just stuff we care about for now
		auto& it = _nbt.Get("Pos").GetList();
		position.pos.x = it[0].GetDouble();
		position.pos.y = it[1].GetDouble();
		position.pos.z = it[2].GetDouble();

		auto& it2 = _nbt.Get("Rotation").GetList();
		rotation.x = it2[0].GetFloat();
		rotation.y = it2[1].GetFloat();

		dimension = static_cast<int8_t>(_nbt.Get("Dimension").GetInt());

		auto& it3 = _nbt.Get("Inventory").GetList();
		for (auto& item : it3) {
			NbtSlotId nbtSlot = item.Get("Slot").GetByte();
			NetworkSlotId networkSlot = inventory.GetNetworkSlotId(nbtSlot);
			if (networkSlot < 0 || networkSlot >= int(inventory.slots.size()))
				continue;
			inventory.slots[size_t(networkSlot)] = ItemStack{ item.Get("id").GetShort(), item.Get("Count").GetByte(),
				                                              item.Get("Damage").GetShort() };
		}
	}

	Tag SerializeToNbt() {
		Tag rootTag;
		rootTag.type = TAG_COMPOUND;
		rootTag.name = "";

		Tag motionTag;
		motionTag.type = TAG_LIST;
		motionTag.name = "Motion";
		motionTag.listType = TAG_DOUBLE;
		Tag sleepTimerTag;
		sleepTimerTag.type = TAG_SHORT;
		sleepTimerTag.name = "SleepTimer";
		sleepTimerTag.shortValue = 0;
		Tag healthTag;
		healthTag.type = TAG_SHORT;
		healthTag.name = "Health";
		healthTag.shortValue = 20;
		Tag airTag;
		airTag.type = TAG_SHORT;
		airTag.name = "Air";
		airTag.shortValue = 300;
		Tag onGroundTag;
		onGroundTag.type = TAG_BYTE;
		onGroundTag.name = "OnGround";
		onGroundTag.byteValue = 0;
		Tag dimensionTag;
		dimensionTag.type = TAG_INT;
		dimensionTag.name = "Dimension";
		dimensionTag.intValue = dimension;
		Tag rotationTag;
		rotationTag.type = TAG_LIST;
		rotationTag.name = "Rotation";
		rotationTag.listType = TAG_FLOAT;
		Tag fallDistanceTag;
		fallDistanceTag.type = TAG_FLOAT;
		fallDistanceTag.name = "FallDistance";
		fallDistanceTag.floatValue = 0.0f;
		Tag sleepingTag;
		sleepingTag.type = TAG_BYTE;
		sleepingTag.name = "Sleeping";
		sleepingTag.byteValue = 0;
		Tag posTag;
		posTag.type = TAG_LIST;
		posTag.name = "Pos";
		posTag.listType = TAG_DOUBLE;
		Tag deathTimeTag;
		deathTimeTag.type = TAG_SHORT;
		deathTimeTag.name = "DeathTime";
		deathTimeTag.shortValue = 0;
		Tag fireTag;
		fireTag.type = TAG_SHORT;
		fireTag.name = "Fire";
		fireTag.shortValue = -20;
		Tag hurtTimeTag;
		hurtTimeTag.type = TAG_SHORT;
		hurtTimeTag.name = "HurtTime";
		hurtTimeTag.shortValue = 0;
		Tag attackTimeTag;
		attackTimeTag.type = TAG_SHORT;
		attackTimeTag.name = "AttackTime";
		attackTimeTag.shortValue = 0;
		Tag inventoryTag;
		inventoryTag.type = TAG_LIST;
		inventoryTag.name = "Inventory";
		inventoryTag.listType = TAG_COMPOUND;

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
		posTag.list.push_back(posX);
		posTag.list.push_back(posY);
		posTag.list.push_back(posZ);

		Tag rotX;
		rotX.type = TAG_FLOAT;
		rotX.floatValue = rotation.x;
		Tag rotY;
		rotY.type = TAG_FLOAT;
		rotY.floatValue = rotation.y;
		rotationTag.list.push_back(rotX);
		rotationTag.list.push_back(rotY);

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
		motionTag.list.push_back(movX);
		motionTag.list.push_back(movY);
		motionTag.list.push_back(movZ);

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
				slotTag.byteValue = inventory.GetNbtSlotId(slotId);
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
				inventoryTag.list.push_back(itemTag);
			}
			slotId++;
		}

		rootTag.compound["Motion"] = motionTag;
		rootTag.compound["SleepTimer"] = sleepTimerTag;
		rootTag.compound["Health"] = healthTag;
		rootTag.compound["Air"] = airTag;
		rootTag.compound["OnGround"] = onGroundTag;
		rootTag.compound["Dimension"] = dimensionTag;
		rootTag.compound["Rotation"] = rotationTag;
		rootTag.compound["FallDistance"] = fallDistanceTag;
		rootTag.compound["Sleeping"] = sleepingTag;
		rootTag.compound["Pos"] = posTag;
		rootTag.compound["DeathTime"] = deathTimeTag;
		rootTag.compound["Fire"] = fireTag;
		rootTag.compound["HurtTime"] = hurtTimeTag;
		rootTag.compound["AttackTime"] = attackTimeTag;
		rootTag.compound["Inventory"] = inventoryTag;

		return rootTag;
	}
};