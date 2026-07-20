/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "serverBlockBehaviors.h"
#include "inventory/interactions/chest.h"
#include "inventory/interactions/crafting_table.h"
#include "inventory/interactions/large_chest.h"

namespace ServerBlock {
BlockBehavior blockBehaviors[256] = {};
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
				partnerChest = _world.GetTileEntityShared<TileEntityChest>({ _position.x - 1, _position.y, _position.z });
			else if (r == BLOCK_CHEST)
				partnerChest = _world.GetTileEntityShared<TileEntityChest>({ _position.x + 1, _position.y, _position.z });
			else if (f == BLOCK_CHEST)
				partnerChest = _world.GetTileEntityShared<TileEntityChest>({ _position.x, _position.y, _position.z - 1 });
			else
				partnerChest = _world.GetTileEntityShared<TileEntityChest>({ _position.x, _position.y, _position.z + 1 });
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
}