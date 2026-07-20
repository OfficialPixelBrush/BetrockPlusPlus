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
void KeepAlive(Packet::KeepAlive& /*pkt*/, PlayerSession& _session) {
	Packet::KeepAlive ka;
	ka.Serialize(_session.stream);
}

void ChatMessage(Packet::ChatMessage& _pkt, PlayerSession& _session, std::vector<std::shared_ptr<PlayerSession>>& _players,
                 WorldManager& _world, CommandManager& _cmd_mgr, std::function<void(PlayerSession&)> _transferDimension) {
	if (_pkt.message.size() > 0 && _pkt.message[0] == '/') {
		_cmd_mgr.Parse(_pkt.message, _session, _world, _transferDimension);
		return;
	}

	Packet::ChatMessage reply;
	reply.message = "<" + _session.username + "> " + _pkt.message;

	// This includes the current session too
	for (auto& receiver : _players) {
		if (receiver->connState != ConnectionState::Playing)
			continue;
		reply.Serialize(receiver->stream);
	}
}

void PlayerMovement(Packet::PlayerMovement& /*pkt*/, PlayerSession& /*session*/) {
	// onGround flag only, so no position update needed.
}

void PlayerPosition(Packet::PlayerPosition& _pkt, PlayerSession& _session) {
	_session.pendingPosition = { _pkt.position.x, _pkt.position.y, _pkt.position.z };
}

void PlayerRotation(Packet::PlayerRotation& _pkt, PlayerSession& _session) {
	_session.rotation.x = _pkt.yaw;
	_session.rotation.y = _pkt.pitch;
}

void PlayerPositionAndRotation(Packet::PlayerPositionAndRotation& _pkt, PlayerSession& _session) {
	_session.pendingPosition = { _pkt.position.x, _pkt.position.y, _pkt.position.z };
	_session.rotation.x = _pkt.yaw;
	_session.rotation.y = _pkt.pitch;
}

void MineBlock(Packet::MineBlock& _pkt, PlayerSession& _session, WorldManager& _world,
               std::vector<std::shared_ptr<PlayerSession>>& /*players*/) {
	Int3 packetPos = { _pkt.position.x, _pkt.position.y, _pkt.position.z };
	switch (_pkt.status) {
	case PacketData::MineStatus::DIGGING_STARTED: {
		_session.startedMiningAtTick = _world.elapsed_ticks;
		BlockType blockId = _world.getBlockId(packetPos);
		_session.lastTargetedBlock = blockId;

		if (Blocks::blockProperties[_session.lastTargetedBlock].hardness == 0.0f) {
			Blocks::BreakAndDropBlock(_world, packetPos);
			return;
		}

		if (Blocks::blockBehaviors[_session.lastTargetedBlock].onBlockClicked) {
			Blocks::blockBehaviors[_session.lastTargetedBlock].onBlockClicked(_world, packetPos);
		}

		ItemStack* heldItem = _session.inventory.getHeldItem();
		if (!heldItem)
			return;
		if (!Items::itemBehavior[heldItem->id].onBlockStartMining)
			return;
		Items::itemBehavior[heldItem->id].onBlockStartMining(_world, heldItem, packetPos, _pkt.face);
		return;
	}
	case PacketData::MineStatus::DIGGING_FINISHED: {
		if (_session.lastTargetedBlock != _world.getBlockId({ _pkt.position.x, _pkt.position.y, _pkt.position.z })) {
			return; // block changed while mining so we don't drop it
		}
		Blocks::BreakAndDropBlock(_world, packetPos);

		ItemStack* heldItem = _session.inventory.getHeldItem();
		if (!heldItem)
			return;
		if (!Items::itemBehavior[heldItem->id].onBlockFinishMining)
			return;
		Items::itemBehavior[heldItem->id].onBlockFinishMining(_world, heldItem, packetPos, _pkt.face);
		return;
	}
	case PacketData::MineStatus::DROPPED_ITEM: {
		ItemStack* heldStack = _session.inventory.getHeldItem();
		if (!heldStack)
			return;

		ItemStack droppedStack = *heldStack;
		droppedStack.count = 1;

		if (_session.entity->dropItem(droppedStack))
			heldStack->decrementCount(1);
		return;
	}
	default:
		return;
	}
	return;
}

void PlaceBlock(Packet::PlaceBlock& _pkt, PlayerSession& _session, WorldManager& _world, Runtime& _gameRuntime) {
	Int3 position = { _pkt.position.x, _pkt.position.y, _pkt.position.z };
	// Block interactions
	auto block = _world.getBlockId(position);

	// Function returns true if we can place a block after running the function
	if (ServerBlock::blockBehaviors[block].onBlockActivated) {
		if (!ServerBlock::blockBehaviors[block].onBlockActivated(_world, position, _session, _gameRuntime))
			return;
	}
	// The server didn't override our block's behavior so check the base behavior
	else if (Blocks::blockBehaviors[block].onBlockActivated) {
		if (!Blocks::blockBehaviors[block].onBlockActivated(_world, position))
			return;
	}

	ItemStack* heldItem = _session.inventory.getHeldItem();
	if (!heldItem)
		return;

	if (_pkt.face == PacketData::FaceDirection::INVALID_USE) {
		// Custom behaviour can be here if needed.
		return;
	}

	if (!Items::IsValid(heldItem->id)) {
		// It's a block
		Int3 placePosition = Blocks::getAdjacentBlockPos(position, _pkt.face);

		auto blockId = BlockType(heldItem->id.value);
		_world.setBlock(placePosition, blockId, heldItem->data);
		auto function = Blocks::blockBehaviors[blockId].onBlockPlaced;
		if (function)
			function(_world, placePosition, *_session.entity, _pkt.face);
		heldItem->decrementCount(1);
	} else {
		// It's an item
		GlobalLogger().info << "Tried to use item\n";
		GlobalLogger().info << position << "\n";
		if (Items::itemBehavior[heldItem->id].onBlockUse) {
			GlobalLogger().info << "Used on " << position << "\n";
			Items::itemBehavior[heldItem->id].onBlockUse(_world, heldItem, position, _pkt.face);
		}
	}
}

void SetHotbarSlot(Packet::SetHotbarSlot& _pkt, PlayerSession& _session) {
	if (_pkt.slot < 0 || _pkt.slot >= 9)
		return;
	_session.inventory.activeHotbarSlot = _pkt.slot;
}

// Click handler
void ClickSlot(Packet::ClickSlot& _pkt, PlayerSession& _session) {
	if (_session.inventoryLocked)
		return;
	_session.pendingWindowId = _pkt.window_id;
	_session.pendingTransactionId = _pkt.transaction_id;

	// The player's inventory is handled seperate
	if (_pkt.window_id == 0) {
		// Make sure what the client thinks and what we have line up
		ItemStack empty{ Items::Id::INVALID };
		auto expected = _session.inventory.getStackInSlot(_pkt.slot_id);
		ItemStack& slotItem = expected ? *expected : empty;
		if (slotItem.id != _pkt.item.id || slotItem.data != _pkt.item.data || slotItem.count != _pkt.item.count) {
			Packet::ContainerTransaction ct;
			ct.accepted = false;
			ct.transaction_id = _session.pendingTransactionId;
			ct.window_id = _pkt.window_id;
			ct.Serialize(_session.stream);
			_session.inventoryLocked = true;

			// Reset the held cursor
			PacketUtilities::sendSlot(_session, -1, -1, &empty);

			PacketUtilities::sendInventory(_session, _pkt.window_id, _session.inventory);
			return;
		}

		// Everything lined up so go as normal
		if (_pkt.right_click) {
			_session.inventoryInteraction.onRightClick(_pkt.slot_id);
			return;
		}
		if (_pkt.shift) {
			_session.inventoryInteraction.onShiftClick(_pkt.slot_id);
			return;
		}
		_session.inventoryInteraction.onLeftClick(_pkt.slot_id);
		return;
	}
	ItemStack empty{ Items::Id::INVALID };
	auto expected = _session.activeInteraction->inventory->getStackInSlot(_pkt.slot_id);
	ItemStack& slotItem = expected ? *expected : empty;
	if (slotItem.id != _pkt.item.id || slotItem.data != _pkt.item.data || slotItem.count != _pkt.item.count) {
		Packet::ContainerTransaction ct;
		ct.accepted = false;
		ct.transaction_id = _session.pendingTransactionId;
		ct.window_id = _pkt.window_id;
		ct.Serialize(_session.stream);
		_session.inventoryLocked = true;

		// Reset the held cursor
		PacketUtilities::sendSlot(_session, -1, -1, &empty);
		PacketUtilities::sendInventory(_session, _pkt.window_id, *_session.activeInteraction->inventory);
		return;
	}

	// Everything lined up so go as normal
	if (_pkt.right_click) {
		_session.activeInteraction->onRightClick(_pkt.slot_id);
		return;
	}
	if (_pkt.shift) {
		_session.activeInteraction->onShiftClick(_pkt.slot_id);
		return;
	}
	_session.activeInteraction->onLeftClick(_pkt.slot_id);
	return;
}

void CloseContainer(Packet::CloseContainer& _pkt, PlayerSession& _session) {
	if (_pkt.window_id == 0 && _session.entity) {
		// Drop the crafting grid items on inventory close
		for (size_t i = 1; i <= 4; i++) {
			ItemStack& stack = _session.inventory.slots[i];
			_session.entity->dropItem(_session.inventory.slots[i]);
			stack = ItemStack{};
		}
	}

	// Get rid of our active interaction and reset the window id
	_session.activeInteraction = nullptr;
	_session.openWindowId = 0;
}

// Client acknowledges a rejected transaction
void ContainerTransaction(Packet::ContainerTransaction& _pkt, PlayerSession& _session) {
	if (_session.inventoryLocked && _pkt.window_id == _session.pendingWindowId &&
	    _pkt.transaction_id == _session.pendingTransactionId) {
		_session.inventoryLocked = false;
	}
}

// Other handlers
void InteractWithEntity(Packet::InteractWithEntity& _pkt, PlayerSession& _session, WorldManager& _world) {
	// Check if session entity and source entity match
	if (_pkt.source_entity_id != _session.entity->id)
		return;

	// Check if target entity exists
	auto entity = _world.entityManager.getEntityByIdShared(_pkt.target_entity_id);
	auto sourceEntity = _world.entityManager.getEntityByIdShared(_pkt.source_entity_id);
	if (!entity || !sourceEntity)
		return;

	if (_pkt.attack) // For funsies hehe
		entity->attackEntityFrom(sourceEntity.get(), 1);

	ItemStack* heldItem = _session.inventory.getHeldItem();
	if (!heldItem)
		return;

	// Get item behavior
	auto iter = Items::itemBehavior.find(heldItem->id);
	if (iter == Items::itemBehavior.end())
		return;

	const auto& behavior = iter->second;

	if (_pkt.attack) {
		if (behavior.onEntityAttack)
			behavior.onEntityAttack(*entity, heldItem);
	} else {
		if (behavior.onEntityUse)
			behavior.onEntityUse(*entity, heldItem);
	}
}

void InteractWithBlock([[maybe_unused]] Packet::InteractWithBlock& _pkt, [[maybe_unused]] PlayerSession& _session,
                       [[maybe_unused]] WorldManager& _world) {}

void Animation(Packet::Animation& _pkt, PlayerSession& _session, EntityTracker& _entityTracker) {
	// Broadcast what we were sent to players who can see this player
	Packet::Animation anim;
	anim.entity_id = _session.entity->id;
	anim.animation = _pkt.animation;
	_entityTracker.sendPacketToViewers(anim, anim.entity_id);
}

void PlayerAction([[maybe_unused]] Packet::PlayerAction& _pkt, [[maybe_unused]] PlayerSession& _session,
                  [[maybe_unused]] EntityTracker& _entityTracker) {
	// Broadcast what we were sent to players who can see this player
}

void Respawn(Packet::Respawn& /*pkt*/, PlayerSession& /*session*/) {
	// TODO: reset position, health, send spawn chunks
}

void UpdateSign(Packet::UpdateSign& _pkt, PlayerSession& _session, WorldManager& _world,
                std::vector<std::shared_ptr<PlayerSession>>& _players) {
	Int3 position = { _pkt.position.x, _pkt.position.y, _pkt.position.z };
	BlockType blockId = _world.getBlockId(position);
	if (blockId != BLOCK_SIGN && blockId != BLOCK_SIGN_WALL)
		return;

	auto tile = std::make_shared<TileEntitySign>(position);

	tile->text1 = _pkt.lines[0];
	tile->text2 = _pkt.lines[1];
	tile->text3 = _pkt.lines[2];
	tile->text4 = _pkt.lines[3];

	_world.createTileEntity(tile);

	for (auto& viewer : _players) {
		if (viewer->connState != ConnectionState::Playing || viewer->dimension != _world.thisDimension)
			continue;

		Int2 chunkCoord = Int2{ static_cast<int32_t>(position.x / 16.0), static_cast<int32_t>(position.z / 16.0) };
		if (!viewer->sentChunks.contains(chunkCoord))
			return;

		_pkt.Serialize(viewer->stream);
	}
}

void Disconnect(Packet::Disconnect& /*pkt*/, PlayerSession& _session) {
	_session.stream.setConnected(false);
}

} // namespace HandlePacket
