/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "../entities/entity_tracker.h"
#include "../blocks/serverBlockBehaviors.h"
#include "handle_packet.h"
#include "blocks.h"
#include "entities/entity_item.h"
#include "inventory/inventory_interaction.h"
#include "inventory/item_stack.h"
#include "packet_utils.h"

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
	std::string broadcast = "<" + session.username + "> " + pkt.message;
	for (auto& other : players) {
		if (other->connState != ConnectionState::Playing)
			continue;
		Packet::ChatMessage reply;
		reply.message = broadcast;
		reply.Serialize(other->stream);
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
	Int3 packetPos = {pkt.position.x, pkt.position.y, pkt.position.z};
	switch (pkt.status) {
	case PacketData::MineStatus::DIGGING_STARTED: {
		session.startedMiningAtTick = world.elapsed_ticks;
		BlockType blockId = world.getBlockId({ pkt.position.x, pkt.position.y, pkt.position.z });
		session.lastTargetedBlock = blockId;

		if (Blocks::blockBehaviors[session.lastTargetedBlock].onBlockClicked) {
			Blocks::blockBehaviors[session.lastTargetedBlock].onBlockClicked(world, packetPos);
		}
		return;
	}
	case PacketData::MineStatus::DIGGING_FINISHED: {
		if (session.lastTargetedBlock != world.getBlockId({ pkt.position.x, pkt.position.y, pkt.position.z })) {
			return; // block changed while mining so we don't drop it
		}
		auto pos = pkt.position;

		BlockType blockId = world.getBlockId({ pos.x, pos.y, pos.z });
		uint8_t meta = world.getMetadata({ pos.x, pos.y, pos.z });
		world.setBlock({ pos.x, pos.y, pos.z }, BLOCK_AIR);

		std::vector<ItemStack> drops = Blocks::getBlockDrops(blockId, meta, world.rand);

		for (ItemStack drop : drops) {
			Vec3 dropPos = { double(pos.x), double(pos.y), double(pos.z) };
			float offset = 0.7f;
			dropPos.x += (world.rand.nextFloat() * offset) + (1.0f - offset) * 0.5;
			dropPos.y += (world.rand.nextFloat() * offset) + (1.0f - offset) * 0.5;
			dropPos.z += (world.rand.nextFloat() * offset) + (1.0f - offset) * 0.5;
			ItemEntity item(dropPos);
			item.itemStack = drop;
			world.entityManager.addEntity(std::make_shared<ItemEntity>(item));
		}
		return;
	}
	case PacketData::MineStatus::DROPPED_ITEM: {
		int count = 1;
		session.entity->dropHeldItem(count);
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

	// NOTE: Also sent for when a block placement is invalid
	if (pkt.face == PacketData::FaceDirection::USE_ITEM) {
		GlobalLogger().info << "Tried to use item\n";
		GlobalLogger().info << position << "\n";
		return;
	}

	Int3 placePosition = position;
	if (pkt.face == PacketData::FaceDirection::Y_MINUS)
		placePosition.y -= 1;
	if (pkt.face == PacketData::FaceDirection::Y_PLUS)
		placePosition.y += 1;
	if (pkt.face == PacketData::FaceDirection::Z_MINUS)
		placePosition.z -= 1;
	if (pkt.face == PacketData::FaceDirection::Z_PLUS)
		placePosition.z += 1;
	if (pkt.face == PacketData::FaceDirection::X_MINUS)
		placePosition.x -= 1;
	if (pkt.face == PacketData::FaceDirection::X_PLUS)
		placePosition.x += 1;

	ItemStack* heldItem = session.inventory.getHeldItem();
	if (!heldItem)
		return;

	// Make sure the block id is valid for placement otherwise we will crash
	if (heldItem->id >= BLOCK_MAX || (heldItem->id <= 0))
		return;

	BlockType blockId = static_cast<BlockType>(heldItem->id.m_value);
	world.setBlock(placePosition, blockId, heldItem->data);
	heldItem->decrementCount(1);
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
		ItemStack empty{ ITEM_INVALID };
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
	ItemStack empty{ ITEM_INVALID };
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

void CloseContainer(Packet::CloseContainer& /*pkt*/, PlayerSession& session) {
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
void InteractWithEntity(Packet::InteractWithEntity& /*pkt*/, PlayerSession& /*session*/) {
	// TODO: attack / interact logic
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

void UpdateSign(Packet::UpdateSign& /*pkt*/, PlayerSession& /*session*/, WorldManager& /*world*/) {
	// TODO: write sign text to world, broadcast to nearby clients
}

void Disconnect(Packet::Disconnect& /*pkt*/, PlayerSession& session) {
	session.stream.setConnected(false);
}

} // namespace HandlePacket