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

void ServerBlock::initialize() {
	// Register unique behaviors here
	blockBehaviors[BLOCK_CRAFTING_TABLE].onBlockActivated = [](WorldManager& world, Int3 position,
	                                                           PlayerSession& session, Runtime& gameRuntime) -> bool {
		Packet::OpenContainer ow;
		ow.window_id = session.getNextWindowId();
		ow.slot_count = 9;
		ow.title = "Crafting";
		ow.window_type = PacketData::WindowType::CRAFTING_TABLE;
		ow.Serialize(session.stream);

		session.activeInteraction = std::make_unique<CraftingTableInventoryInteraction>(&session.inventory, world,
		                                                                                gameRuntime, position);
		session.activeInteraction->initSnapshot();
		return false;
	};
	blockBehaviors[BLOCK_CHEST].onBlockActivated = [](WorldManager& world, Int3 position, PlayerSession& session,
	                                                  Runtime& gameRuntime) -> bool {
		auto chest = world.getTileEntityShared<TileEntityChest>(position);
		if (!chest) {
			chest = std::make_shared<TileEntityChest>(position);
			world.createTileEntity(chest);
		}

		// Are we a double chest?
		auto l = world.getBlockId({ position.x - 1, position.y, position.z });
		auto r = world.getBlockId({ position.x + 1, position.y, position.z });
		auto f = world.getBlockId({ position.x, position.y, position.z - 1 });
		auto b = world.getBlockId({ position.x, position.y, position.z + 1 });
		bool doubleChest = (l == BLOCK_CHEST || r == BLOCK_CHEST || f == BLOCK_CHEST || b == BLOCK_CHEST);

		if (doubleChest) {
			std::shared_ptr<TileEntityChest> partnerChest = nullptr;
			if (l == BLOCK_CHEST)
				partnerChest = world.getTileEntityShared<TileEntityChest>({ position.x - 1, position.y, position.z });
			else if (r == BLOCK_CHEST)
				partnerChest = world.getTileEntityShared<TileEntityChest>({ position.x + 1, position.y, position.z });
			else if (f == BLOCK_CHEST)
				partnerChest = world.getTileEntityShared<TileEntityChest>({ position.x, position.y, position.z - 1 });
			else
				partnerChest = world.getTileEntityShared<TileEntityChest>({ position.x, position.y, position.z + 1 });
			if (!partnerChest)
				return false;

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
			return false;
		}

		// Setup interaction
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
		return false;
	};
}