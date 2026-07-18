/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "handle_packet.h"
#include "../blocks/serverBlockBehaviors.h"
#include "../entities/entity_tracker.h"
#include "blocks.h"
#include "blocks/block_properties.h"
#include "entities/entity_item.h"
#include "inventory/inventory_interaction.h"
#include "inventory/item_stack.h"
#include "items/item_properties.h"
#include "packet_utils.h"
#include "tile_entities/tile_entity.h"
#include <cstring>
#include <memory>

namespace HandlePacket {
void KeepAlive(Packet::KeepAlive& /*pkt*/, PlayerSession& session) {
	Packet::KeepAlive ka;
	ka.Serialize(session.stream);
}

void ChatMessage(Packet::ChatMessage& pkt, PlayerSession& session, std::vector<std::shared_ptr<PlayerSession>>& players,
                 WorldManager& world, CommandManager& cmd_mgr, std::function<void(PlayerSession&)> transferDimension) {
	if (pkt.message.size() > 0 && pkt.message[0] == '/') {
		cmd_mgr.Parse(pkt.message, session, world, transferDimension);
		return;
	}

	Packet::ChatMessage reply;
	reply.message = "<" + session.username + "> " + pkt.message;

	// This includes the current session too
	for (auto& receiver : players) {
		if (receiver->connState != ConnectionState::Playing)
			continue;
		reply.Serialize(receiver->stream);
	}
}

void PlayerMovement(Packet::PlayerMovement& /*pkt*/, PlayerSession& /*session*/) {
	// onGround flag only, so no position update needed.
}

void PlayerPosition(Packet::PlayerPosition& pkt, PlayerSession& session) {
	session.position.pos = pkt.position;
}

void PlayerRotation(Packet::PlayerRotation& pkt, PlayerSession& session) {
	session.rotation.x = pkt.yaw;
	session.rotation.y = pkt.pitch;
}

void PlayerPositionAndRotation(Packet::PlayerPositionAndRotation& pkt, PlayerSession& session) {
	session.position.pos = pkt.position;
	session.rotation.x = pkt.yaw;
	session.rotation.y = pkt.pitch;
}

void MineBlock(Packet::MineBlock& pkt, PlayerSession& session, WorldManager& world,
               std::vector<std::shared_ptr<PlayerSession>>& /*players*/) {
	Int3 packetPos = { pkt.position.x, pkt.position.y, pkt.position.z };
	switch (pkt.status) {
	case PacketData::MineStatus::DIGGING_STARTED: {
		session.startedMiningAtTick = world.elapsed_ticks;
		BlockType blockId = world.getBlockId(packetPos);
		session.lastTargetedBlock = blockId;

		if (Blocks::blockProperties[session.lastTargetedBlock].hardness == 0.0f) {
			Blocks::BreakAndDropBlock(world, packetPos);
			return;
		}

		if (Blocks::blockBehaviors[session.lastTargetedBlock].onBlockClicked) {
			Blocks::blockBehaviors[session.lastTargetedBlock].onBlockClicked(world, packetPos);
		}
		return;
	}
	case PacketData::MineStatus::DIGGING_FINISHED: {
		if (session.lastTargetedBlock != world.getBlockId({ pkt.position.x, pkt.position.y, pkt.position.z })) {
			return; // block changed while mining so we don't drop it
		}
		Blocks::BreakAndDropBlock(world, packetPos);
		return;
	}
	case PacketData::MineStatus::DROPPED_ITEM: {
		ItemStack* heldStack = session.inventory.getHeldItem();
		if (!heldStack)
			return;

		ItemStack droppedStack = *heldStack;
		droppedStack.count = 1;

		if (session.entity->dropItem(droppedStack))
			heldStack->decrementCount(1);
		return;
	}
	default:
		return;
	}
	return;
}

void PlaceBlock(Packet::PlaceBlock& pkt, PlayerSession& session, WorldManager& world, Runtime& gameRuntime) {
	Int3 position = { pkt.position.x, pkt.position.y, pkt.position.z };
	// Block interactions
	auto block = world.getBlockId(position);

	// Function returns true if we can place a block after running the function
	if (ServerBlock::blockBehaviors[block].onBlockActivated) {
		if (!ServerBlock::blockBehaviors[block].onBlockActivated(world, position, session, gameRuntime))
			return;
	}
	// The server didn't override our block's behavior so check the base behavior
	else if (Blocks::blockBehaviors[block].onBlockActivated) {
		if (!Blocks::blockBehaviors[block].onBlockActivated(world, position))
			return;
	}

	ItemStack* heldItem = session.inventory.getHeldItem();
	if (!heldItem)
		return;

	if (pkt.face == PacketData::FaceDirection::INVALID_USE) {
		// Custom behaviour can be here if needed.
		return;
	}

	if (!Items::IsValid(heldItem->id)) {
		// It's a block
		Int3 placePosition = Blocks::getAdjacentBlockPos(position, pkt.face);

		auto blockId = BlockType(heldItem->id.m_value);
		world.setBlock(placePosition, blockId, heldItem->data);
		auto function = Blocks::blockBehaviors[blockId].onBlockPlaced;
		if (function)
			function(world, placePosition, *session.entity, pkt.face);
		heldItem->decrementCount(1);
	} else {
		// It's an item
		GlobalLogger().info << "Tried to use item\n";
		GlobalLogger().info << position << "\n";
		if (Items::itemBehavior[heldItem->id].onBlockUse) {
			GlobalLogger().info << "Used on " << position << "\n";
			Items::itemBehavior[heldItem->id].onBlockUse(world, heldItem, position, pkt.face);
		}
	}
}

void SetHotbarSlot(Packet::SetHotbarSlot& pkt, PlayerSession& session) {
	if (pkt.slot < 0 || pkt.slot >= 9)
		return;
	session.inventory.activeHotbarSlot = pkt.slot;
}

// Click handler
void ClickSlot(Packet::ClickSlot& pkt, PlayerSession& session) {
	if (session.inventoryLocked)
		return;
	session.pendingWindowId = pkt.window_id;
	session.pendingTransactionId = pkt.transaction_id;

	// The player's inventory is handled seperate
	if (pkt.window_id == 0) {
		// Make sure what the client thinks and what we have line up
		ItemStack empty{ Items::Id::INVALID };
		auto expected = session.inventory.getStackInSlot(pkt.slot_id);
		ItemStack& slotItem = expected ? *expected : empty;
		if (slotItem.id != pkt.item.id || slotItem.data != pkt.item.data || slotItem.count != pkt.item.count) {
			Packet::ContainerTransaction ct;
			ct.accepted = false;
			ct.transaction_id = session.pendingTransactionId;
			ct.window_id = pkt.window_id;
			ct.Serialize(session.stream);
			session.inventoryLocked = true;

			// Reset the held cursor
			PacketUtilities::sendSlot(session, -1, -1, &empty);

			PacketUtilities::sendInventory(session, pkt.window_id, session.inventory);
			return;
		}

		// Everything lined up so go as normal
		if (pkt.right_click) {
			session.inventoryInteraction.onRightClick(pkt.slot_id);
			return;
		}
		if (pkt.shift) {
			session.inventoryInteraction.onShiftClick(pkt.slot_id);
			return;
		}
		session.inventoryInteraction.onLeftClick(pkt.slot_id);
		return;
	}
	ItemStack empty{ Items::Id::INVALID };
	auto expected = session.activeInteraction->inventory->getStackInSlot(pkt.slot_id);
	ItemStack& slotItem = expected ? *expected : empty;
	if (slotItem.id != pkt.item.id || slotItem.data != pkt.item.data || slotItem.count != pkt.item.count) {
		Packet::ContainerTransaction ct;
		ct.accepted = false;
		ct.transaction_id = session.pendingTransactionId;
		ct.window_id = pkt.window_id;
		ct.Serialize(session.stream);
		session.inventoryLocked = true;

		// Reset the held cursor
		PacketUtilities::sendSlot(session, -1, -1, &empty);
		PacketUtilities::sendInventory(session, pkt.window_id, *session.activeInteraction->inventory);
		return;
	}

	// Everything lined up so go as normal
	if (pkt.right_click) {
		session.activeInteraction->onRightClick(pkt.slot_id);
		return;
	}
	if (pkt.shift) {
		session.activeInteraction->onShiftClick(pkt.slot_id);
		return;
	}
	session.activeInteraction->onLeftClick(pkt.slot_id);
	return;
}

void CloseContainer(Packet::CloseContainer& pkt, PlayerSession& session) {
	if (pkt.window_id == 0 && session.entity) {
		// Drop the crafting grid items on inventory close
		for (size_t i = 1; i <= 4; i++) {
			ItemStack& stack = session.inventory.slots[i];
			session.entity->dropItem(session.inventory.slots[i]);
			stack = ItemStack{};
		}
	}

	// Get rid of our active interaction and reset the window id
	session.activeInteraction = nullptr;
	session.openWindowId = 0;
}

// Client acknowledges a rejected transaction
void ContainerTransaction(Packet::ContainerTransaction& pkt, PlayerSession& session) {
	if (session.inventoryLocked && pkt.window_id == session.pendingWindowId &&
	    pkt.transaction_id == session.pendingTransactionId) {
		session.inventoryLocked = false;
	}
}

// Other handlers
void InteractWithEntity(Packet::InteractWithEntity& pkt, PlayerSession& session, WorldManager& world) {
	// Check if session entity and source entity match
	if (pkt.source_entity_id != session.entity->id)
		return;

	// Check if target entity exists
	auto entity = world.entityManager.getEntityByIdShared(pkt.target_entity_id);
	auto sourceEntity = world.entityManager.getEntityByIdShared(pkt.source_entity_id);
	if (!entity || !sourceEntity)
		return;

	if (pkt.attack) // For funsies hehe
		entity->attackEntityFrom(sourceEntity.get(), 1);

	const ItemStack* heldItem = session.inventory.getHeldItem();
	if (!heldItem)
		return;

	// Get item behavior
	auto iter = Items::itemBehavior.find(heldItem->id);
	if (iter == Items::itemBehavior.end())
		return;

	const auto& behavior = iter->second;

	if (pkt.attack) {
		if (behavior.onEntityAttack)
			behavior.onEntityAttack(*entity, heldItem->id);
	} else {
		if (behavior.onEntityUse)
			behavior.onEntityUse(*entity);
	}
}

void InteractWithBlock([[maybe_unused]] Packet::InteractWithBlock& pkt, [[maybe_unused]] PlayerSession& session,
                       [[maybe_unused]] WorldManager& world) {}

void Animation(Packet::Animation& pkt, PlayerSession& session, EntityTracker& entityTracker) {
	// Broadcast what we were sent to players who can see this player
	Packet::Animation anim;
	anim.entity_id = session.entity->id;
	anim.animation = pkt.animation;
	entityTracker.sendPacketToViewers(anim, anim.entity_id);
}

void PlayerAction([[maybe_unused]] Packet::PlayerAction& pkt, [[maybe_unused]] PlayerSession& session,
                  [[maybe_unused]] EntityTracker& entityTracker) {
	// Broadcast what we were sent to players who can see this player
}

void Respawn(Packet::Respawn& /*pkt*/, PlayerSession& /*session*/) {
	// TODO: reset position, health, send spawn chunks
}

void UpdateSign(Packet::UpdateSign& pkt, PlayerSession& session, WorldManager& world,
                std::vector<std::shared_ptr<PlayerSession>>& players) {
	Int3 position = { pkt.position.x, pkt.position.y, pkt.position.z };
	BlockType blockId = world.getBlockId(position);
	if (blockId != BLOCK_SIGN && blockId != BLOCK_SIGN_WALL)
		return;

	auto tile = std::make_shared<TileEntitySign>(position);

	tile->m_text1 = pkt.lines[0];
	tile->m_text2 = pkt.lines[1];
	tile->m_text3 = pkt.lines[2];
	tile->m_text4 = pkt.lines[3];

	world.createTileEntity(tile);

	for (auto& viewer : players) {
		if (viewer->connState != ConnectionState::Playing || viewer->dimension != world.thisDimension)
			continue;

		Int2 chunkCoord = Int2{ static_cast<int32_t>(position.x / 16.0), static_cast<int32_t>(position.z / 16.0) };
		if (!viewer->sentChunks.contains(chunkCoord))
			return;

		pkt.Serialize(viewer->stream);
	}
}

void Disconnect(Packet::Disconnect& /*pkt*/, PlayerSession& session) {
	session.stream.setConnected(false);
}

} // namespace HandlePacket
