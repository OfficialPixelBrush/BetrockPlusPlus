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
#include "server.h"
#include "tile_entities/tile_entity.h"
#include <cstring>
#include <memory>

namespace HandlePacket {
void KeepAlive(Packet::KeepAlive& /*pkt*/, PlayerSession& _session) {
	Packet::KeepAlive ka;
	ka.Serialize(_session.stream);
}

void ChatMessage(Packet::ChatMessage& _pkt, PlayerSession& _session,
                 std::vector<std::shared_ptr<PlayerSession>>& _players, WorldManager& _world, CommandManager& _cmdMgr,
                 std::function<void(PlayerSession&)> _transferDimension) {
	if (_pkt.message.size() > 0 && _pkt.message[0] == '/') {
		_cmdMgr.Parse(_pkt.message, _session, _world, _transferDimension);
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

void PlayerMovement(Packet::PlayerMovement& _pkt, PlayerSession& _session) {
	// On ground flag from client
	if (_session.entity) {
		_session.entity->onGround = _pkt.onGround;
		_session.entity->HandlePositionChecks();
	}
}

void PlayerPosition(Packet::PlayerPosition& _pkt, PlayerSession& _session) {
	_session.pendingPosition = { _pkt.position.x, _pkt.position.y, _pkt.position.z };
	if (_session.entity) {
		_session.entity->onGround = _pkt.onGround;
		_session.entity->HandlePositionChecks();
	}
}

void PlayerRotation(Packet::PlayerRotation& _pkt, PlayerSession& _session) {
	_session.rotation.x = _pkt.yaw;
	_session.rotation.y = _pkt.pitch;
	_session.pendingPosition = _session.position.pos;
	if (_session.entity) {
		_session.entity->onGround = _pkt.onGround;
		_session.entity->HandlePositionChecks();
	}
}

void PlayerPositionAndRotation(Packet::PlayerPositionAndRotation& _pkt, PlayerSession& _session) {
	_session.pendingPosition = { _pkt.position.x, _pkt.position.y, _pkt.position.z };
	_session.rotation.x = _pkt.yaw;
	_session.rotation.y = _pkt.pitch;
	if (_session.entity) {
		_session.entity->onGround = _pkt.onGround;
		_session.entity->HandlePositionChecks();
	}
}

void MineBlock(Packet::MineBlock& _pkt, PlayerSession& _session, WorldManager& _world,
               std::vector<std::shared_ptr<PlayerSession>>& /*players*/) {
	Int3 packetPos = { _pkt.position.x, _pkt.position.y, _pkt.position.z };
	switch (_pkt.status) {
	case PacketData::MineStatus::DIGGING_STARTED: {
		_session.startedMiningAtTick = _world.elapsedTicks;
		BlockType blockId = _world.GetBlockId(packetPos);
		_session.lastTargetedBlock = blockId;

		if (Blocks::blockProperties[_session.lastTargetedBlock].hardness == 0.0f) {
			Blocks::BreakAndDropBlock(_world, packetPos);
			return;
		}

		if (Blocks::blockBehaviors[_session.lastTargetedBlock].onBlockClicked) {
			Blocks::blockBehaviors[_session.lastTargetedBlock].onBlockClicked(_world, packetPos);
		}

		ItemStack* heldItem = _session.inventory.GetHeldItem();
		if (!heldItem)
			return;
		if (!Items::itemBehavior[heldItem->id].onBlockStartMining)
			return;
		Items::itemBehavior[heldItem->id].onBlockStartMining(_world, heldItem, packetPos, _pkt.face);
		return;
	}
	case PacketData::MineStatus::DIGGING_FINISHED: {
		if (_session.lastTargetedBlock != _world.GetBlockId({ _pkt.position.x, _pkt.position.y, _pkt.position.z })) {
			return; // block changed while mining so we don't drop it
		}
		Blocks::BreakAndDropBlock(_world, packetPos);

		ItemStack* heldItem = _session.inventory.GetHeldItem();
		if (!heldItem)
			return;
		if (!Items::itemBehavior[heldItem->id].onBlockFinishMining)
			return;
		Items::itemBehavior[heldItem->id].onBlockFinishMining(_world, heldItem, packetPos, _pkt.face);
		return;
	}
	case PacketData::MineStatus::DROPPED_ITEM: {
		ItemStack* heldStack = _session.inventory.GetHeldItem();
		if (!heldStack)
			return;

		ItemStack droppedStack = *heldStack;
		droppedStack.count = 1;

		if (_session.entity->DropItem(droppedStack))
			heldStack->DecrementCount(1);
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
	auto block = _world.GetBlockId(position);

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

	ItemStack* heldItem = _session.inventory.GetHeldItem();
	if (!heldItem)
		return;

	if (_pkt.face == PacketData::FaceDirection::INVALID_USE) {
		// Custom behaviour can be here if needed.
		return;
	}

	if (!Items::IsValid(heldItem->id)) {
		// It's a block
		Int3 placePosition = Blocks::GetAdjacentBlockPos(position, _pkt.face);
		if (heldItem->id.value < 0 || heldItem->id.value >= 256) {
			// invalid
			return;
		}

		auto blockId = BlockType(heldItem->id.value);

		// Can we place this block here?
		auto entitiesInBlock = _world.entityManager.GetEntitiesWithinAabb(
		    { double(placePosition.x), double(placePosition.y), double(placePosition.z), double(placePosition.x) + 1.0,
		      double(placePosition.y) + 1.0, double(placePosition.z) + 1.0 });

		// Check to see if any entities overlap our block's collider
		bool collided = false;
		if (Blocks::blockProperties[blockId].isCollidable) {
			auto blockCollider = Blocks::blockBehaviors[blockId]
			                         .getCollider(/*meta=*/0)
			                         .Offset(
			                             placePosition.x, placePosition.y,
			                             placePosition.z); // Block colliders are at the origin so shift to world space
			for (auto& entity : entitiesInBlock) {
				collided = blockCollider.Intersects(entity->collider) && entity->preventEntitySpawning;
				if (collided)
					break;
			}
		}

		// We can place the block here
		if (!collided) {
			_world.SetBlock(placePosition, blockId, heldItem->data);
			auto function = Blocks::blockBehaviors[blockId].onBlockPlaced;
			if (function)
				function(_world, placePosition, *_session.entity, _pkt.face);
			heldItem->DecrementCount(1);
		}
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
	// Update previously held item
	auto previousHeldItem = _session.inventory.GetHeldItem();
	if (previousHeldItem)
		if (auto fn = Items::itemBehavior[previousHeldItem->id].onStopHolding) {
			fn(previousHeldItem);
		}
	// Move to new slot
	_session.inventory.activeHotbarSlot = _pkt.slot;
	// Update held item start holding
	// TODO: This doesn't update when an item is given or picked up!!!!
	auto heldItem = _session.inventory.GetHeldItem();
	if (heldItem)
		if (auto fn = Items::itemBehavior[heldItem->id].onStartHolding) {
			fn(heldItem);
		}
}

// Click handler
void ClickSlot(Packet::ClickSlot& _pkt, PlayerSession& _session) {
	if (_session.inventoryLocked)
		return;
	_session.pendingWindowId = _pkt.windowId;
	_session.pendingTransactionId = _pkt.transactionId;

	// The player's inventory is handled seperate
	if (_pkt.windowId == 0) {
		// Make sure what the client thinks and what we have line up
		ItemStack empty{ Items::Id::INVALID };
		auto expected = _session.inventory.GetStackInSlot(_pkt.slotId);
		ItemStack& slotItem = expected ? *expected : empty;
		if (slotItem.id != _pkt.item.id || slotItem.data != _pkt.item.data || slotItem.count != _pkt.item.count) {
			Packet::ContainerTransaction ct;
			ct.accepted = false;
			ct.transactionId = _session.pendingTransactionId;
			ct.windowId = _pkt.windowId;
			ct.Serialize(_session.stream);
			_session.inventoryLocked = true;

			// Reset the held cursor
			PacketUtilities::SendSlot(_session, -1, -1, &empty);

			PacketUtilities::SendInventory(_session, _pkt.windowId, _session.inventory);
			return;
		}

		// Everything lined up so go as normal
		if (_pkt.rightClick) {
			_session.inventoryInteraction.OnRightClick(_pkt.slotId);
			return;
		}
		if (_pkt.shift) {
			_session.inventoryInteraction.OnShiftClick(_pkt.slotId);
			return;
		}
		_session.inventoryInteraction.OnLeftClick(_pkt.slotId);
		return;
	}
	ItemStack empty{ Items::Id::INVALID };
	auto expected = _session.activeInteraction->inventory->GetStackInSlot(_pkt.slotId);
	ItemStack& slotItem = expected ? *expected : empty;
	if (slotItem.id != _pkt.item.id || slotItem.data != _pkt.item.data || slotItem.count != _pkt.item.count) {
		Packet::ContainerTransaction ct;
		ct.accepted = false;
		ct.transactionId = _session.pendingTransactionId;
		ct.windowId = _pkt.windowId;
		ct.Serialize(_session.stream);
		_session.inventoryLocked = true;

		// Reset the held cursor
		PacketUtilities::SendSlot(_session, -1, -1, &empty);
		PacketUtilities::SendInventory(_session, _pkt.windowId, *_session.activeInteraction->inventory);
		return;
	}

	// Everything lined up so go as normal
	if (_pkt.rightClick) {
		_session.activeInteraction->OnRightClick(_pkt.slotId);
		return;
	}
	if (_pkt.shift) {
		_session.activeInteraction->OnShiftClick(_pkt.slotId);
		return;
	}
	_session.activeInteraction->OnLeftClick(_pkt.slotId);
	return;
}

void CloseContainer(Packet::CloseContainer& _pkt, PlayerSession& _session) {
	if (_pkt.windowId == 0 && _session.entity) {
		// Drop the crafting grid items on inventory close
		for (size_t i = 1; i <= 4; i++) {
			ItemStack& stack = _session.inventory.slots[i];
			_session.entity->DropItem(_session.inventory.slots[i]);
			stack = ItemStack{};
		}
	}

	// Get rid of our active interaction and reset the window id
	_session.activeInteraction = nullptr;
	_session.openWindowId = 0;
}

// Client acknowledges a rejected transaction
void ContainerTransaction(Packet::ContainerTransaction& _pkt, PlayerSession& _session) {
	if (_session.inventoryLocked && _pkt.windowId == _session.pendingWindowId &&
	    _pkt.transactionId == _session.pendingTransactionId) {
		_session.inventoryLocked = false;
	}
}

// Other handlers
void InteractWithEntity(Packet::InteractWithEntity& _pkt, PlayerSession& _session, WorldManager& _world) {
	// Check if session entity and source entity match
	if (_pkt.sourceEntityId != _session.entity->id)
		return;

	// Check if target entity exists
	auto entity = _world.entityManager.GetEntityByIdShared(_pkt.targetEntityId);
	auto sourceEntity = _world.entityManager.GetEntityByIdShared(_pkt.sourceEntityId);
	if (!entity || !sourceEntity)
		return;

	if (_pkt.attack) // For funsies hehe
		entity->AttackEntityFrom(sourceEntity.get(), 1);

	ItemStack* heldItem = _session.inventory.GetHeldItem();
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
	anim.entityId = _session.entity->id;
	anim.animation = _pkt.animation;
	_entityTracker.SendPacketToViewers(anim, anim.entityId);
}

void PlayerAction([[maybe_unused]] Packet::PlayerAction& _pkt, [[maybe_unused]] PlayerSession& _session,
                  [[maybe_unused]] EntityTracker& _entityTracker) {
	// Broadcast what we were sent to players who can see this player
}

void Respawn(Packet::Respawn& _pkt, PlayerSession& _session, Server& _server) {
	auto targetDim = _pkt.dimension;

	// Send a respawn packet to confirm
	Packet::Respawn pkt;
	pkt.dimension = targetDim;
	pkt.Serialize(_session.stream);

	// Detach the old entity from whatever dimension it died in
	auto& oldEntity = _session.entity;
	auto oldWorld = _server.GetWorldForDimension(Dimension(_session.dimension));

	// unregister the old entity
	oldEntity->session = nullptr; // fully detach

	// Build the replacement entity
	auto newEntity = std::make_shared<EntityMPPlayer>();
	newEntity->session = &_session;
	newEntity->id = oldEntity->id;
	newEntity->dim = targetDim;

	_session.entity = newEntity;

	// Force a refresh
	_session.dimension = targetDim;
	_session.entityTracker = targetDim == 0 ? &_server.overworldEntityTracker : &_server.hellEntityTracker;

	// Get our spawn point
	auto world = _server.GetWorldForDimension(targetDim);
	auto spawn = world->GetSpawnPoint(/*Random Adjust=*/true);
	Vec3 spawnLocation = { double(spawn.x) + 0.5, double(spawn.y) + PLAYER_EYE_HEIGHT + 1 / 64, double(spawn.z) + 0.5 };

	// Position the new entity then register it
	newEntity->Teleport(spawnLocation);
	world->entityManager.AddEntity(_session.entity, newEntity->id);

	// Update our session position
	_session.position.pos = spawnLocation;
	_session.pendingTeleport = spawnLocation;
	_session.pendingPosition.reset();

	// Tell our client to tp
	Packet::PlayerPositionAndRotation tpPkt;
	tpPkt.position.x = spawnLocation.x;
	tpPkt.position.y = spawnLocation.y;
	tpPkt.cameraY = spawnLocation.y + PLAYER_EYE_HEIGHT;
	tpPkt.position.z = spawnLocation.z;
	tpPkt.yaw = 0;
	tpPkt.pitch = 0;
	tpPkt.onGround = false;
	tpPkt.Serialize(_session.stream);
}

void UpdateSign(Packet::UpdateSign& _pkt, PlayerSession& _session, WorldManager& _world,
                std::vector<std::shared_ptr<PlayerSession>>& _players) {
	Int3 position = { _pkt.position.x, _pkt.position.y, _pkt.position.z };
	BlockType blockId = _world.GetBlockId(position);
	if (blockId != BLOCK_SIGN && blockId != BLOCK_SIGN_WALL)
		return;

	auto tile = std::make_shared<TileEntitySign>(position);

	tile->text1 = _pkt.lines[0];
	tile->text2 = _pkt.lines[1];
	tile->text3 = _pkt.lines[2];
	tile->text4 = _pkt.lines[3];

	_world.CreateTileEntity(tile);

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
	_session.stream.SetConnected(false);
}

} // namespace HandlePacket