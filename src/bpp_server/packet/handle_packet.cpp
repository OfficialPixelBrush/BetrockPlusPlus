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
	ka.Serialize(session.m_stream);
}

void ChatMessage(Packet::ChatMessage& pkt, PlayerSession& session, std::vector<std::shared_ptr<PlayerSession>>& players,
                 WorldManager& world, CommandManager& cmd_mgr, std::function<void(PlayerSession&)> transferDimension) {
	if (pkt.m_message.size() > 0 && pkt.m_message[0] == '/') {
		cmd_mgr.Parse(pkt.m_message, session, world, transferDimension);
		return;
	}

	Packet::ChatMessage reply;
	reply.m_message = "<" + session.m_username + "> " + pkt.m_message;

	// This includes the current session too
	for (auto& receiver : players) {
		if (receiver->m_connState != ConnectionState::Playing)
			continue;
		reply.Serialize(receiver->m_stream);
	}
}

void PlayerMovement(Packet::PlayerMovement& /*pkt*/, PlayerSession& /*session*/) {
	// onGround flag only, so no position update needed.
}

void PlayerPosition(Packet::PlayerPosition& pkt, PlayerSession& session) {
	session.m_pendingPosition = { pkt.m_position.m_x, pkt.m_position.m_y, pkt.m_position.m_z };
}

void PlayerRotation(Packet::PlayerRotation& pkt, PlayerSession& session) {
	session.m_rotation.m_x = pkt.m_yaw;
	session.m_rotation.m_y = pkt.m_pitch;
}

void PlayerPositionAndRotation(Packet::PlayerPositionAndRotation& pkt, PlayerSession& session) {
	session.m_pendingPosition = { pkt.m_position.m_x, pkt.m_position.m_y, pkt.m_position.m_z };
	session.m_rotation.m_x = pkt.m_yaw;
	session.m_rotation.m_y = pkt.m_pitch;
}

void MineBlock(Packet::MineBlock& pkt, PlayerSession& session, WorldManager& world,
               std::vector<std::shared_ptr<PlayerSession>>& /*players*/) {
	Int3 packetPos = { pkt.m_position.m_x, pkt.m_position.m_y, pkt.m_position.m_z };
	switch (pkt.m_status) {
	case PacketData::MineStatus::DIGGING_STARTED: {
		session.m_startedMiningAtTick = world.m_elapsed_ticks;
		BlockType blockId = world.getBlockId(packetPos);
		session.m_lastTargetedBlock = blockId;

		if (Blocks::blockProperties[session.m_lastTargetedBlock].m_hardness == 0.0f) {
			Blocks::BreakAndDropBlock(world, packetPos);
			return;
		}

		if (Blocks::blockBehaviors[session.m_lastTargetedBlock].m_onBlockClicked) {
			Blocks::blockBehaviors[session.m_lastTargetedBlock].m_onBlockClicked(world, packetPos);
		}

		ItemStack* heldItem = session.m_inventory.getHeldItem();
		if (!heldItem)
			return;
		if (!Items::itemBehavior[heldItem->m_id].m_onBlockStartMining)
			return;
		Items::itemBehavior[heldItem->m_id].m_onBlockStartMining(world, heldItem, packetPos, pkt.m_face);
		return;
	}
	case PacketData::MineStatus::DIGGING_FINISHED: {
		if (session.m_lastTargetedBlock != world.getBlockId({ pkt.m_position.m_x, pkt.m_position.m_y, pkt.m_position.m_z })) {
			return; // block changed while mining so we don't drop it
		}
		Blocks::BreakAndDropBlock(world, packetPos);

		ItemStack* heldItem = session.m_inventory.getHeldItem();
		if (!heldItem)
			return;
		if (!Items::itemBehavior[heldItem->m_id].m_onBlockFinishMining)
			return;
		Items::itemBehavior[heldItem->m_id].m_onBlockFinishMining(world, heldItem, packetPos, pkt.m_face);
		return;
	}
	case PacketData::MineStatus::DROPPED_ITEM: {
		ItemStack* heldStack = session.m_inventory.getHeldItem();
		if (!heldStack)
			return;

		ItemStack droppedStack = *heldStack;
		droppedStack.m_count = 1;

		if (session.m_entity->dropItem(droppedStack))
			heldStack->decrementCount(1);
		return;
	}
	default:
		return;
	}
	return;
}

void PlaceBlock(Packet::PlaceBlock& pkt, PlayerSession& session, WorldManager& world, Runtime& gameRuntime) {
	Int3 position = { pkt.m_position.m_x, pkt.m_position.m_y, pkt.m_position.m_z };
	// Block interactions
	auto block = world.getBlockId(position);

	// Function returns true if we can place a block after running the function
	if (ServerBlock::blockBehaviors[block].m_onBlockActivated) {
		if (!ServerBlock::blockBehaviors[block].m_onBlockActivated(world, position, session, gameRuntime))
			return;
	}
	// The server didn't override our block's behavior so check the base behavior
	else if (Blocks::blockBehaviors[block].m_onBlockActivated) {
		if (!Blocks::blockBehaviors[block].m_onBlockActivated(world, position))
			return;
	}

	ItemStack* heldItem = session.m_inventory.getHeldItem();
	if (!heldItem)
		return;

	if (pkt.m_face == PacketData::FaceDirection::INVALID_USE) {
		// Custom behaviour can be here if needed.
		return;
	}

	if (!Items::IsValid(heldItem->m_id)) {
		// It's a block
		Int3 placePosition = Blocks::getAdjacentBlockPos(position, pkt.m_face);

		auto blockId = BlockType(heldItem->m_id.m_value);
		world.setBlock(placePosition, blockId, heldItem->m_data);
		auto function = Blocks::blockBehaviors[blockId].m_onBlockPlaced;
		if (function)
			function(world, placePosition, *session.m_entity, pkt.m_face);
		heldItem->decrementCount(1);
	} else {
		// It's an item
		GlobalLogger().m_info << "Tried to use item\n";
		GlobalLogger().m_info << position << "\n";
		if (Items::itemBehavior[heldItem->m_id].m_onBlockUse) {
			GlobalLogger().m_info << "Used on " << position << "\n";
			Items::itemBehavior[heldItem->m_id].m_onBlockUse(world, heldItem, position, pkt.m_face);
		}
	}
}

void SetHotbarSlot(Packet::SetHotbarSlot& pkt, PlayerSession& session) {
	if (pkt.m_slot < 0 || pkt.m_slot >= 9)
		return;
	session.m_inventory.m_activeHotbarSlot = pkt.m_slot;
}

// Click handler
void ClickSlot(Packet::ClickSlot& pkt, PlayerSession& session) {
	if (session.m_inventoryLocked)
		return;
	session.m_pendingWindowId = pkt.m_window_id;
	session.m_pendingTransactionId = pkt.m_transaction_id;

	// The player's inventory is handled seperate
	if (pkt.m_window_id == 0) {
		// Make sure what the client thinks and what we have line up
		ItemStack empty{ Items::Id::INVALID };
		auto expected = session.m_inventory.getStackInSlot(pkt.m_slot_id);
		ItemStack& slotItem = expected ? *expected : empty;
		if (slotItem.m_id != pkt.m_item.m_id || slotItem.m_data != pkt.m_item.m_data || slotItem.m_count != pkt.m_item.m_count) {
			Packet::ContainerTransaction ct;
			ct.m_accepted = false;
			ct.m_transaction_id = session.m_pendingTransactionId;
			ct.m_window_id = pkt.m_window_id;
			ct.Serialize(session.m_stream);
			session.m_inventoryLocked = true;

			// Reset the held cursor
			PacketUtilities::sendSlot(session, -1, -1, &empty);

			PacketUtilities::sendInventory(session, pkt.m_window_id, session.m_inventory);
			return;
		}

		// Everything lined up so go as normal
		if (pkt.m_right_click) {
			session.m_inventoryInteraction.onRightClick(pkt.m_slot_id);
			return;
		}
		if (pkt.m_shift) {
			session.m_inventoryInteraction.onShiftClick(pkt.m_slot_id);
			return;
		}
		session.m_inventoryInteraction.onLeftClick(pkt.m_slot_id);
		return;
	}
	ItemStack empty{ Items::Id::INVALID };
	auto expected = session.m_activeInteraction->m_inventory->getStackInSlot(pkt.m_slot_id);
	ItemStack& slotItem = expected ? *expected : empty;
	if (slotItem.m_id != pkt.m_item.m_id || slotItem.m_data != pkt.m_item.m_data || slotItem.m_count != pkt.m_item.m_count) {
		Packet::ContainerTransaction ct;
		ct.m_accepted = false;
		ct.m_transaction_id = session.m_pendingTransactionId;
		ct.m_window_id = pkt.m_window_id;
		ct.Serialize(session.m_stream);
		session.m_inventoryLocked = true;

		// Reset the held cursor
		PacketUtilities::sendSlot(session, -1, -1, &empty);
		PacketUtilities::sendInventory(session, pkt.m_window_id, *session.m_activeInteraction->m_inventory);
		return;
	}

	// Everything lined up so go as normal
	if (pkt.m_right_click) {
		session.m_activeInteraction->onRightClick(pkt.m_slot_id);
		return;
	}
	if (pkt.m_shift) {
		session.m_activeInteraction->onShiftClick(pkt.m_slot_id);
		return;
	}
	session.m_activeInteraction->onLeftClick(pkt.m_slot_id);
	return;
}

void CloseContainer(Packet::CloseContainer& pkt, PlayerSession& session) {
	if (pkt.m_window_id == 0 && session.m_entity) {
		// Drop the crafting grid items on inventory close
		for (size_t i = 1; i <= 4; i++) {
			ItemStack& stack = session.m_inventory.m_slots[i];
			session.m_entity->dropItem(session.m_inventory.m_slots[i]);
			stack = ItemStack{};
		}
	}

	// Get rid of our active interaction and reset the window id
	session.m_activeInteraction = nullptr;
	session.m_openWindowId = 0;
}

// Client acknowledges a rejected transaction
void ContainerTransaction(Packet::ContainerTransaction& pkt, PlayerSession& session) {
	if (session.m_inventoryLocked && pkt.m_window_id == session.m_pendingWindowId &&
	    pkt.m_transaction_id == session.m_pendingTransactionId) {
		session.m_inventoryLocked = false;
	}
}

// Other handlers
void InteractWithEntity(Packet::InteractWithEntity& pkt, PlayerSession& session, WorldManager& world) {
	// Check if session entity and source entity match
	if (pkt.m_source_entity_id != session.m_entity->m_id)
		return;

	// Check if target entity exists
	auto entity = world.m_entityManager.getEntityByIdShared(pkt.m_target_entity_id);
	auto sourceEntity = world.m_entityManager.getEntityByIdShared(pkt.m_source_entity_id);
	if (!entity || !sourceEntity)
		return;

	if (pkt.m_attack) // For funsies hehe
		entity->attackEntityFrom(sourceEntity.get(), 1);

	ItemStack* heldItem = session.m_inventory.getHeldItem();
	if (!heldItem)
		return;

	// Get item behavior
	auto iter = Items::itemBehavior.find(heldItem->m_id);
	if (iter == Items::itemBehavior.end())
		return;

	const auto& behavior = iter->second;

	if (pkt.m_attack) {
		if (behavior.m_onEntityAttack)
			behavior.m_onEntityAttack(*entity, heldItem);
	} else {
		if (behavior.m_onEntityUse)
			behavior.m_onEntityUse(*entity, heldItem);
	}
}

void InteractWithBlock([[maybe_unused]] Packet::InteractWithBlock& pkt, [[maybe_unused]] PlayerSession& session,
                       [[maybe_unused]] WorldManager& world) {}

void Animation(Packet::Animation& pkt, PlayerSession& session, EntityTracker& entityTracker) {
	// Broadcast what we were sent to players who can see this player
	Packet::Animation anim;
	anim.m_entity_id = session.m_entity->m_id;
	anim.m_animation = pkt.m_animation;
	entityTracker.sendPacketToViewers(anim, anim.m_entity_id);
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
	Int3 position = { pkt.m_position.m_x, pkt.m_position.m_y, pkt.m_position.m_z };
	BlockType blockId = world.getBlockId(position);
	if (blockId != BLOCK_SIGN && blockId != BLOCK_SIGN_WALL)
		return;

	auto tile = std::make_shared<TileEntitySign>(position);

	tile->m_text1 = pkt.m_lines[0];
	tile->m_text2 = pkt.m_lines[1];
	tile->m_text3 = pkt.m_lines[2];
	tile->m_text4 = pkt.m_lines[3];

	world.createTileEntity(tile);

	for (auto& viewer : players) {
		if (viewer->m_connState != ConnectionState::Playing || viewer->m_dimension != world.m_thisDimension)
			continue;

		Int2 chunkCoord = Int2{ static_cast<int32_t>(position.m_x / 16.0), static_cast<int32_t>(position.m_z / 16.0) };
		if (!viewer->m_sentChunks.contains(chunkCoord))
			return;

		pkt.Serialize(viewer->m_stream);
	}
}

void Disconnect(Packet::Disconnect& /*pkt*/, PlayerSession& session) {
	session.m_stream.setConnected(false);
}

} // namespace HandlePacket
