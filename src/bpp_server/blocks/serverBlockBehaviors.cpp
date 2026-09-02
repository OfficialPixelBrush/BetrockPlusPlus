/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "serverBlockBehaviors.h"
#include "blocks.h"
#include "inventory/interactions/chest.h"
#include "inventory/interactions/crafting_table.h"
#include "inventory/interactions/furnace.h"
#include "inventory/interactions/large_chest.h"
#include "tile_entities/tile_entity.h"
#include "../../bpp_shared/helpers/direction_fixer.h"

namespace ServerBlock {
BlockBehavior blockBehaviors[BLOCK_MAX] = {};
} // namespace ServerBlock

void ServerBlock::Initialize() {
	// Register unique behaviors here
	blockBehaviors[BLOCK_CRAFTING_TABLE].onBlockActivated = [](WorldManager& _world, Int3 _position,
	                                                           PlayerSession& _session, Runtime& _gameRuntime) -> bool {
		Packet::OpenContainer ow;
		ow.windowId = _session.GetNextWindowId();
		ow.slotCount = 9;
		ow.title = "Crafting";
		ow.windowType = PacketData::WindowType::CRAFTING_TABLE;
		ow.Serialize(_session.stream);

		_session.activeInteraction = std::make_unique<CraftingTableInventoryInteraction>(&_session.inventory, _world,
		                                                                                 _gameRuntime, _position);
		_session.activeInteraction->InitSnapshot();
		return false;
	};

	auto furnaceActivated = [](WorldManager& _world, Int3 _position, PlayerSession& _session,
	                           Runtime& _gameRuntime) -> bool {
		auto furnace = _world.GetTileEntityShared<TileEntityFurnace>(_position);
		if (!furnace)
			return false;

		Packet::OpenContainer ow;
		ow.windowId = _session.GetNextWindowId();
		ow.slotCount = 3;
		ow.title = "Furnace";
		ow.windowType = PacketData::WindowType::FURNACE;
		ow.Serialize(_session.stream);

		_session.activeInteraction = std::make_unique<FurnaceInventoryInteraction>(&_session.inventory, furnace);
		_session.activeInteraction->InitSnapshot();

		PacketUtilities::SendInventory(_session, _session.openWindowId, *_session.activeInteraction->inventory);

		return false;
	};

	blockBehaviors[BLOCK_FURNACE].onBlockActivated = furnaceActivated;
	blockBehaviors[BLOCK_FURNACE_LIT].onBlockActivated = furnaceActivated;

	blockBehaviors[BLOCK_CHEST].onBlockActivated = [](WorldManager& _world, Int3 _position, PlayerSession& _session,
	                                                  Runtime& _gameRuntime) -> bool {
		auto chest = _world.GetTileEntityShared<TileEntityChest>(_position);
		if (!chest) {
			return false;
		}

		// Are we a double chest?
		auto l = _world.GetBlockId({ _position.x - 1, _position.y, _position.z });
		auto r = _world.GetBlockId({ _position.x + 1, _position.y, _position.z });
		auto f = _world.GetBlockId({ _position.x, _position.y, _position.z - 1 });
		auto b = _world.GetBlockId({ _position.x, _position.y, _position.z + 1 });
		bool doubleChest = (l == BLOCK_CHEST || r == BLOCK_CHEST || f == BLOCK_CHEST || b == BLOCK_CHEST);

		if (doubleChest) {
			std::shared_ptr<TileEntityChest> partnerChest = nullptr;
			if (l == BLOCK_CHEST)
				partnerChest = _world.GetTileEntityShared<TileEntityChest>(
				    { _position.x - 1, _position.y, _position.z });
			else if (r == BLOCK_CHEST)
				partnerChest = _world.GetTileEntityShared<TileEntityChest>(
				    { _position.x + 1, _position.y, _position.z });
			else if (f == BLOCK_CHEST)
				partnerChest = _world.GetTileEntityShared<TileEntityChest>(
				    { _position.x, _position.y, _position.z - 1 });
			else
				partnerChest = _world.GetTileEntityShared<TileEntityChest>(
				    { _position.x, _position.y, _position.z + 1 });
			if (!partnerChest)
				return false;

			bool isLeftSide = (r == BLOCK_CHEST || b == BLOCK_CHEST);
			if (!isLeftSide)
				std::swap(chest, partnerChest);

			Packet::OpenContainer ow;
			ow.windowId = _session.GetNextWindowId();
			ow.slotCount = 54;
			ow.title = "Large Chest";
			ow.windowType = PacketData::WindowType::CHEST;
			ow.Serialize(_session.stream);

			_session.activeInteraction = std::make_unique<LargeChestInventoryInteraction>(&_session.inventory, chest,
			                                                                              partnerChest);
			_session.activeInteraction->InitSnapshot();

			PacketUtilities::SendInventory(_session, _session.openWindowId, *_session.activeInteraction->inventory);
			return false;
		}

		// Setup interaction
		_session.activeInteraction = std::make_unique<ChestInventoryInteraction>(&_session.inventory, chest);
		_session.activeInteraction->InitSnapshot();

		// Single chest
		// Open the chest window
		Packet::OpenContainer ow;
		ow.windowId = _session.GetNextWindowId();
		ow.slotCount = 27;
		ow.title = "Chest";
		ow.windowType = PacketData::WindowType::CHEST;
		ow.Serialize(_session.stream);

		// Send inventory
		PacketUtilities::SendInventory(_session, _session.openWindowId, *_session.activeInteraction->inventory);
		return false;
	};

	blockBehaviors[BLOCK_JUKEBOX].onBlockActivated = [](WorldManager& _world, Int3 _position, PlayerSession& _session,
	                                                    Runtime& _gameRuntime) -> bool {
		//ItemStack* heldItem = _session.inventory.GetHeldItem();
		//if (!heldItem)
		//	return false;
		// TODO: Check if jukebox is already playing
		//if (!IsRecord(heldItem.id) && )
		//	return false;
		if (auto fn = _world.onWorldEvent) {
			fn(PacketData::WorldEvent::RECORD_PLAY, _position, Items::Id::RECORD_CAT, nullptr);
			//fn(PacketData::WorldEvent::RECORD_PLAY, _position, 0);
		}
		return false;
	};
	blockBehaviors[BLOCK_BED].onBlockActivated = [](WorldManager& _world, Int3 _position, PlayerSession& _session,
	                                                    Runtime& _gameRuntime) -> bool {
		if (!_world.InBounds(_position.y))
			return false;
		// Already sleeping? Don't let the client re-trigger the interaction.
		if (_session.entity->isSleeping)
			return false;
		// If not head, but foot, move to headboard
		{
			auto meta = _world.GetMetadata(_position);
			if (!(meta & 0b1000)) {
				auto dir = GetDirectionFromMeta(BLOCK_BED, meta);
				_position.Offset(dir);
			}
		}
		Packet::InteractWithBlock pkt;
		pkt.entityId = _session.entity->id;
		pkt.interactionId = PacketData::BlockInteraction::SLEEPING;
		pkt.position = { _position.x, static_cast<int8_t>(_position.y), _position.z };
		pkt.Serialize(_session.stream);
		Packet::Animation anim;
		anim.entityId = _session.entity->id;
		anim.animation = PacketData::Animation::PUNCH;
		_session.entityTracker->SendPacketToViewers(anim, _session.entity->id);
		_session.entityTracker->SendPacketToViewers(pkt, _session.entity->id);
		_session.entity->isSleeping = true;
		// TODO: Find proper spawn position!
		_session.spawnPosition = _position.Offset(Direction::Value::Up);
		return false;
	};
}