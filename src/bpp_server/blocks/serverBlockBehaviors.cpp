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
	blockBehaviors[BLOCK_CRAFTING_TABLE].m_onBlockActivated = [](WorldManager& world, Int3 position,
	                                                           PlayerSession& session, Runtime& gameRuntime) -> bool {
		Packet::OpenContainer ow;
		ow.m_window_id = session.getNextWindowId();
		ow.m_slot_count = 9;
		ow.m_title = "Crafting";
		ow.m_window_type = PacketData::WindowType::CRAFTING_TABLE;
		ow.Serialize(session.m_stream);

		session.m_activeInteraction = std::make_unique<CraftingTableInventoryInteraction>(&session.m_inventory, world,
		                                                                                gameRuntime, position);
		session.m_activeInteraction->initSnapshot();
		return false;
	};
	blockBehaviors[BLOCK_CHEST].m_onBlockActivated = [](WorldManager& world, Int3 position, PlayerSession& session,
	                                                  Runtime& gameRuntime) -> bool {
		auto chest = world.getTileEntityShared<TileEntityChest>(position);
		if (!chest) {
			return false;
		}

		// Are we a double chest?
		auto l = world.getBlockId({ position.m_x - 1, position.m_y, position.m_z });
		auto r = world.getBlockId({ position.m_x + 1, position.m_y, position.m_z });
		auto f = world.getBlockId({ position.m_x, position.m_y, position.m_z - 1 });
		auto b = world.getBlockId({ position.m_x, position.m_y, position.m_z + 1 });
		bool doubleChest = (l == BLOCK_CHEST || r == BLOCK_CHEST || f == BLOCK_CHEST || b == BLOCK_CHEST);

		if (doubleChest) {
			std::shared_ptr<TileEntityChest> partnerChest = nullptr;
			if (l == BLOCK_CHEST)
				partnerChest = world.getTileEntityShared<TileEntityChest>({ position.m_x - 1, position.m_y, position.m_z });
			else if (r == BLOCK_CHEST)
				partnerChest = world.getTileEntityShared<TileEntityChest>({ position.m_x + 1, position.m_y, position.m_z });
			else if (f == BLOCK_CHEST)
				partnerChest = world.getTileEntityShared<TileEntityChest>({ position.m_x, position.m_y, position.m_z - 1 });
			else
				partnerChest = world.getTileEntityShared<TileEntityChest>({ position.m_x, position.m_y, position.m_z + 1 });
			if (!partnerChest)
				return false;

			bool isLeftSide = (r == BLOCK_CHEST || b == BLOCK_CHEST);
			if (!isLeftSide)
				std::swap(chest, partnerChest);

			Packet::OpenContainer ow;
			ow.m_window_id = session.getNextWindowId();
			ow.m_slot_count = 54;
			ow.m_title = "Large Chest";
			ow.m_window_type = PacketData::WindowType::CHEST;
			ow.Serialize(session.m_stream);

			session.m_activeInteraction = std::make_unique<LargeChestInventoryInteraction>(&session.m_inventory, chest,
			                                                                             partnerChest);
			session.m_activeInteraction->initSnapshot();

			PacketUtilities::sendInventory(session, session.m_openWindowId, *session.m_activeInteraction->m_inventory);
			return false;
		}

		// Setup interaction
		session.m_activeInteraction = std::make_unique<ChestInventoryInteraction>(&session.m_inventory, chest);
		session.m_activeInteraction->initSnapshot();

		// Single chest
		// Open the chest window
		Packet::OpenContainer ow;
		ow.m_window_id = session.getNextWindowId();
		ow.m_slot_count = 27;
		ow.m_title = "Chest";
		ow.m_window_type = PacketData::WindowType::CHEST;
		ow.Serialize(session.m_stream);

		// Send inventory
		PacketUtilities::sendInventory(session, session.m_openWindowId, *session.m_activeInteraction->m_inventory);
		return false;
	};
}