/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "handle_packet.h"

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
	std::wstring broadcast = L"<" + session.username + L"> " + pkt.message;
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

// Returns true if it's safe to accept pos as the client's new position right now.
static bool AcceptClientPosition(PlayerSession& session, const Vec3& pos) {
	if (!session.awaitingTeleportAck)
		return true;

	Vec3 d = { pos.x - session.pendingTeleportPos.x, pos.y - session.pendingTeleportPos.y,
		       pos.z - session.pendingTeleportPos.z };
	if ((d.x * d.x + d.y * d.y + d.z * d.z) < 1.0) {
		// Client has caught up to the teleport - resume trusting normal movement.
		session.awaitingTeleportAck = false;
		return true;
	}

	// Still a stale pre-teleport packet in flight - ignore it.
	return false;
}

void PlayerPosition(Packet::PlayerPosition& pkt, PlayerSession& session) {
	if (!AcceptClientPosition(session, pkt.position))
		return;
	session.position.pos = pkt.position;
}

void PlayerRotation(Packet::PlayerRotation& pkt, PlayerSession& session) {
	session.rotation.x = pkt.yaw;
	session.rotation.y = pkt.pitch;
}

void PlayerPositionAndRotation(Packet::PlayerPositionAndRotation& pkt, PlayerSession& session) {
	if (!AcceptClientPosition(session, pkt.position))
		return;
	session.position.pos = pkt.position;
	session.rotation.x = pkt.yaw;
	session.rotation.y = pkt.pitch;
}

void MineBlock(Packet::MineBlock& pkt, PlayerSession& session, WorldManager& world,
               std::vector<std::shared_ptr<PlayerSession>>& /*players*/) {
	if (pkt.status != 2)
		return;
	auto pos = pkt.position;
	// Use last known good position
	double dx = session.entity->prevPosX;
	double dy = session.entity->prevPosY;
	double dz = session.entity->prevPosZ;
	double distance = dx * dx + dy * dy + dz * dz;
	if (distance > 36.0) {
		return; // more than 6 blocks away so we drop it
	}

	// TODO: make it so when you break stone with your fist it doesn't drop (based on tool you are holding)
	BlockType blockId = world.getBlockId({ pos.x, pos.y, pos.z });
	uint8_t meta = world.getMetadata({ pos.x, pos.y, pos.z });
	world.setBlock({ pos.x, pos.y, pos.z }, BLOCK_AIR);

	std::vector<ItemStack> drops = Blocks::getBlockDrops(blockId, meta, world.rand);

	// TODO: spawn an item instead of adding it to your inventory
	for (ItemStack drop : drops) {
		// hotbar makes up slots 36-44 so try that first
		if (session.inventory.mergeItemStackInInventory(drop, false, 36, 44) ||
		    session.inventory.mergeItemStackInInventory(drop, false, 9, 35)) {
			PacketUtilities::sendInventory(session, session.openWindowId, session.inventory);
		}
	}
}

void PlaceBlock(Packet::PlaceBlock& pkt, PlayerSession& session, WorldManager& world,
                std::vector<std::shared_ptr<PlayerSession>>& /*players*/) {
	// Block interactions
	auto block = world.getBlockId({ pkt.position.x, pkt.position.y, pkt.position.z });
	if (block == BLOCK_CHEST) {
		// Are we a double chest?
		auto l = world.getBlockId({ pkt.position.x - 1, pkt.position.y, pkt.position.z });
		auto r = world.getBlockId({ pkt.position.x + 1, pkt.position.y, pkt.position.z });
		auto f = world.getBlockId({ pkt.position.x, pkt.position.y, pkt.position.z - 1 });
		auto b = world.getBlockId({ pkt.position.x, pkt.position.y, pkt.position.z + 1 });
		bool doubleChest = (l == BLOCK_CHEST || r == BLOCK_CHEST || f == BLOCK_CHEST || b == BLOCK_CHEST);

		if (doubleChest) {
			auto chest = world.getTileEntityShared<TileEntityChest>({ pkt.position.x, pkt.position.y, pkt.position.z });
			if (!chest)
				return;

			std::shared_ptr<TileEntityChest> partnerChest = nullptr;
			if (l == BLOCK_CHEST)
				partnerChest = world.getTileEntityShared<TileEntityChest>(
				    { pkt.position.x - 1, pkt.position.y, pkt.position.z });
			else if (r == BLOCK_CHEST)
				partnerChest = world.getTileEntityShared<TileEntityChest>(
				    { pkt.position.x + 1, pkt.position.y, pkt.position.z });
			else if (f == BLOCK_CHEST)
				partnerChest = world.getTileEntityShared<TileEntityChest>(
				    { pkt.position.x, pkt.position.y, pkt.position.z - 1 });
			else
				partnerChest = world.getTileEntityShared<TileEntityChest>(
				    { pkt.position.x, pkt.position.y, pkt.position.z + 1 });
			if (!partnerChest)
				return;

			bool isLeftSide = (r == BLOCK_CHEST || b == BLOCK_CHEST);
			if (!isLeftSide)
				std::swap(chest, partnerChest);

			Packet::OpenContainer ow;
			ow.window_id = session.getNextWindowId();
			ow.slot_count = 54;
			ow.title = "Large Chest";
			ow.window_type = PacketData::WindowType::CHEST;
			ow.Serialize(session.stream);

			session.activeInteraction = std::make_unique<LargeChestInventoryInteraction>(&session.inventory, chest,
			                                                                             partnerChest);
			session.activeInteraction->initSnapshot();

			PacketUtilities::sendInventory(session, session.openWindowId, *session.activeInteraction->inventory);
			return;
		}

		// Setup interaction
		auto chest = world.getTileEntityShared<TileEntityChest>({ pkt.position.x, pkt.position.y, pkt.position.z });
		if (!chest)
			return; // not a chest tile entity
		session.activeInteraction = std::make_unique<ChestInventoryInteraction>(&session.inventory, chest);
		session.activeInteraction->initSnapshot();

		// Single chest
		// Open the chest window
		Packet::OpenContainer ow;
		ow.window_id = session.getNextWindowId();
		ow.slot_count = 27;
		ow.title = "Chest";
		ow.window_type = PacketData::WindowType::CHEST;
		ow.Serialize(session.stream);

		// Send inventory
		PacketUtilities::sendInventory(session, session.openWindowId, *session.activeInteraction->inventory);
		return;
	}

	auto pos = pkt.position;
	// NOTE: Also sent for when a block placement is invalid
	if (pkt.face == PacketData::FaceDirection::USE_ITEM) {
		GlobalLogger().info << "Tried to use item\n";
		GlobalLogger().info << pkt.position << "\n";
	}
	if (pkt.face == PacketData::FaceDirection::Y_MINUS)
		pos.y -= 1;
	if (pkt.face == PacketData::FaceDirection::Y_PLUS)
		pos.y += 1;
	if (pkt.face == PacketData::FaceDirection::Z_MINUS)
		pos.z -= 1;
	if (pkt.face == PacketData::FaceDirection::Z_PLUS)
		pos.z += 1;
	if (pkt.face == PacketData::FaceDirection::X_MINUS)
		pos.x -= 1;
	if (pkt.face == PacketData::FaceDirection::X_PLUS)
		pos.x += 1;
	// Make sure the block id is valid for placement otherwise we will crash
	if (pkt.item.id < BLOCK_MAX && (pkt.item.id >= 0))
		world.setBlock({ pos.x, pos.y, pos.z }, BlockType(pkt.item.id), pkt.item.data);
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

void InteractWithBlock(Packet::InteractWithBlock& pkt, PlayerSession& session, WorldManager& world) {}

void Animation(Packet::Animation& pkt, PlayerSession& session, std::vector<std::shared_ptr<PlayerSession>>& players) {
	for (auto& other : players) {
		if (other.get() == &session)
			continue;
		if (other->connState != ConnectionState::Playing)
			continue;
		Packet::Animation anim;
		anim.entity_id = session.entityId;
		anim.animation = pkt.animation;
		anim.Serialize(other->stream);
	}
}

void PlayerAction(Packet::PlayerAction& /*pkt*/, PlayerSession& /*session*/) {
	// TODO: sneak/sleep state changes
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