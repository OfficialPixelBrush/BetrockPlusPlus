/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "item_properties.h"
#include "base_types.h"
#include "blocks.h"
#include "blocks/block_properties.h"
#include "blocks/materials.h"
#include "enums/items.h"
#include "inventory/item_stack.h"
#include "item_map.h"
#include "logger.h"
#include "packet_data.h"
#include "player_conn/player_session.h"
#include "server.h"
#include "tool_properties.h"
#include "world/world.h"
#include <cstdint>

namespace Items {

// Global table definitions; declared extern in the header
std::unordered_map<ItemId, ItemBehavior> itemBehavior = {};
std::unordered_map<ItemId, ItemProperties> itemProperties = {};

bool IsValid(ItemId _id) {
	return ((_id >= Items::Id::SHOVEL_IRON && _id < Items::Id::MAX) ||
	        (_id >= Items::Id::RECORD_13 && _id < Items::Id::RECORD_MAX));
}

constexpr bool IsArmor(ItemId _id) {
	return (_id >= Items::HELMET_LEATHER && _id <= Items::BOOTS_GOLD);
}

constexpr bool IsThrowable(ItemId _id) {
	return (_id == Items::SNOWBALL || _id == Items::EGG);
}

constexpr bool IsBlock(ItemId _id) {
	return (_id > 0 && _id <= Items::THRESHOLD);
}

bool IsStackable(ItemId _id) {
	return Items::GetMaxStack(_id) > 1;
}

int32_t GetMaxStack(ItemId _id) {
	// Stack size 1
	switch (_id) {
		// Food (ItemFood sets maxStackSize=1 in constructor)
	case Items::APPLE:
	case Items::APPLE_GOLDEN:
	case Items::BREAD:
	case Items::PORKCHOP:
	case Items::PORKCHOP_COOKED:
	case Items::FISH:
	case Items::FISH_COOKED:
	case Items::MUSHROOM_STEW: // ItemSoup extends ItemFood

		// Containers / vehicles / misc unstackables
	case Items::CAKE: // ItemReed.setMaxStackSize(1)
	case Items::BED:  // ItemBed.setMaxStackSize(1)
	case Items::SADDLE:
	case Items::BUCKET:
	case Items::BUCKET_WATER:
	case Items::BUCKET_LAVA:
	case Items::BUCKET_MILK:
	case Items::MINECART:
	case Items::MINECART_CHEST:
	case Items::MINECART_FURNACE:
	case Items::BOAT:
	case Items::DOOR_WOOD:
	case Items::DOOR_IRON:
	case Items::SIGN:       // ItemSign
	case Items::MAP:        // ItemMap.setMaxStackSize(1)
	case Items::RECORD_13:  // ItemRecord
	case Items::RECORD_CAT: // ItemRecord
		return 1;

	default:
		break;
	}

	// Tools, weapons, armor all set maxStackSize=1 in their constructors
	if (IsTool(_id) || IsWeapon(_id) || IsArmor(_id))
		return 1;

	// Stack size 16
	if (_id == Items::SNOWBALL || _id == Items::EGG)
		return 16;

	if (_id == Items::COOKIE)
		return 8;

	// Item, ItemCoal, ItemSeeds, ItemRedstone, ItemDye, ItemPainting,
	// ItemReed (sugarcane & repeater item), ItemRecord (never reached above),
	// all blocks, and any resource item not listed above.
	return Items::STACK_MAX;
}

void RegisterAll() {
	itemBehavior[Items::Id::HOE_WOOD] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_STONE] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_IRON] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_GOLD] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_DIAMOND] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::FLINT_AND_STEEL] = ItemBehavior{ .onBlockStartMining = TestSetGoal,
		                                                     .onBlockUse = UseFlintAndSteel };

	// Tool Properties
	// Sword
	toolProperties[Items::Id::SWORD_WOOD] = ToolProperties{ .type = ToolType::Sword,
		                                                    .material = ToolMaterial::Wooden,
		                                                    .canHarvest = CanShearsOrSwordHarvest };
	toolProperties[Items::Id::SWORD_STONE] = ToolProperties{ .type = ToolType::Sword,
		                                                     .material = ToolMaterial::Stone,
		                                                     .canHarvest = CanShearsOrSwordHarvest };
	toolProperties[Items::Id::SWORD_IRON] = ToolProperties{ .type = ToolType::Sword,
		                                                    .material = ToolMaterial::Iron,
		                                                    .canHarvest = CanShearsOrSwordHarvest };
	toolProperties[Items::Id::SWORD_GOLD] = ToolProperties{ .type = ToolType::Sword,
		                                                    .material = ToolMaterial::Gold,
		                                                    .canHarvest = CanShearsOrSwordHarvest };
	toolProperties[Items::Id::SWORD_DIAMOND] = ToolProperties{ .type = ToolType::Sword,
		                                                       .material = ToolMaterial::Diamond,
		                                                       .canHarvest = CanShearsOrSwordHarvest };
	// Pickaxe
	toolProperties[Items::Id::PICKAXE_WOOD] = ToolProperties{
		.type = ToolType::Pickaxe,
		.material = ToolMaterial::Wooden,
	};
	toolProperties[Items::Id::PICKAXE_STONE] = ToolProperties{
		.type = ToolType::Pickaxe,
		.material = ToolMaterial::Stone,
	};
	toolProperties[Items::Id::PICKAXE_IRON] = ToolProperties{
		.type = ToolType::Pickaxe,
		.material = ToolMaterial::Iron,
	};
	toolProperties[Items::Id::PICKAXE_GOLD] = ToolProperties{
		.type = ToolType::Pickaxe,
		.material = ToolMaterial::Gold,
	};
	toolProperties[Items::Id::PICKAXE_DIAMOND] = ToolProperties{
		.type = ToolType::Pickaxe,
		.material = ToolMaterial::Diamond,
	};
	// Axe
	toolProperties[Items::Id::AXE_WOOD] = ToolProperties{
		.type = ToolType::Axe,
		.material = ToolMaterial::Wooden,
	};
	toolProperties[Items::Id::AXE_STONE] = ToolProperties{
		.type = ToolType::Axe,
		.material = ToolMaterial::Stone,
	};
	toolProperties[Items::Id::AXE_IRON] = ToolProperties{
		.type = ToolType::Axe,
		.material = ToolMaterial::Iron,
	};
	toolProperties[Items::Id::AXE_GOLD] = ToolProperties{
		.type = ToolType::Axe,
		.material = ToolMaterial::Gold,
	};
	toolProperties[Items::Id::AXE_DIAMOND] = ToolProperties{
		.type = ToolType::Axe,
		.material = ToolMaterial::Diamond,
	};
	// Shovel
	toolProperties[Items::Id::SHOVEL_WOOD] = ToolProperties{ .type = ToolType::Shovel,
		                                                     .material = ToolMaterial::Wooden,
		                                                     .canHarvest = CanShovelHarvest };
	toolProperties[Items::Id::SHOVEL_STONE] = ToolProperties{ .type = ToolType::Shovel,
		                                                      .material = ToolMaterial::Stone,
		                                                      .canHarvest = CanShovelHarvest };
	toolProperties[Items::Id::SHOVEL_IRON] = ToolProperties{ .type = ToolType::Shovel,
		                                                     .material = ToolMaterial::Iron,
		                                                     .canHarvest = CanShovelHarvest };
	toolProperties[Items::Id::SHOVEL_GOLD] = ToolProperties{ .type = ToolType::Shovel,
		                                                     .material = ToolMaterial::Gold,
		                                                     .canHarvest = CanShovelHarvest };
	toolProperties[Items::Id::SHOVEL_DIAMOND] = ToolProperties{ .type = ToolType::Shovel,
		                                                        .material = ToolMaterial::Diamond,
		                                                        .canHarvest = CanShovelHarvest };
	// Hoe
	toolProperties[Items::Id::HOE_WOOD] = ToolProperties{
		.type = ToolType::Hoe,
		.material = ToolMaterial::Wooden,
	};
	toolProperties[Items::Id::HOE_STONE] = ToolProperties{
		.type = ToolType::Hoe,
		.material = ToolMaterial::Stone,
	};
	toolProperties[Items::Id::HOE_IRON] = ToolProperties{
		.type = ToolType::Hoe,
		.material = ToolMaterial::Iron,
	};
	toolProperties[Items::Id::HOE_GOLD] = ToolProperties{
		.type = ToolType::Hoe,
		.material = ToolMaterial::Gold,
	};
	toolProperties[Items::Id::HOE_DIAMOND] = ToolProperties{
		.type = ToolType::Hoe,
		.material = ToolMaterial::Diamond,
	};
	// Apply max uses based on material
	for (auto& toolProperty : toolProperties) {
		toolProperty.second.maxUses = GetMaterialUses(toolProperty.second.material);
	}
	// Misc tools
	toolProperties[Items::Id::FLINT_AND_STEEL] = ToolProperties{ .type = ToolType::None,
		                                                         .material = ToolMaterial::None,
		                                                         .maxUses = DURABILITY_FLINT_AND_STEEL };
	toolProperties[Items::Id::SHEARS] = ToolProperties{ .type = ToolType::None,
		                                                .material = ToolMaterial::None,
		                                                .maxUses = DURABILITY_SHEARS,
		                                                .canHarvest = CanShearsOrSwordHarvest };
	toolProperties[Items::Id::BOW] = ToolProperties{ .type = ToolType::None,
		                                             .material = ToolMaterial::None,
		                                             .maxUses = DURABILITY_BOW };
	toolProperties[Items::Id::FISHING_ROD] = ToolProperties{ .type = ToolType::None,
		                                                     .material = ToolMaterial::None,
		                                                     .maxUses = DURABILITY_FISHING_ROD };

	// Item behaviors
	for (auto& toolProperty : toolProperties) {
		// Apply sword behavior to all swords
		switch (toolProperty.second.type) {
		case ToolType::Sword:
			itemBehavior[toolProperty.first] = ItemBehavior{ .onEntityAttack = AttackWithItem };
			continue;
		case ToolType::Pickaxe:
			continue;
		case ToolType::Axe:
			continue;
		case ToolType::Shovel:
			continue;
		case ToolType::Hoe:
			itemBehavior[toolProperty.first] = ItemBehavior{ .onBlockUse = UseHoe };
			continue;
		default:
			continue;
		}
	}

	itemBehavior[SUGARCANE].onBlockUse = [](WorldManager& _world, ItemStack* _stack, Int3 _pos,
	                                        PacketData::FaceDirection _face) {
		Int3 placePos = Blocks::GetAdjacentBlockPos(_pos, _face);
		if (!Blocks::CanSugarcaneSurviveAt(_world, placePos))
			return;

		_world.SetBlock(placePos, BLOCK_SUGARCANE);
		_stack->DecrementCount(1);
	};

	itemBehavior[SIGN].onBlockUse = [](WorldManager& _world, ItemStack* _stack, Int3 _pos,
	                                   PacketData::FaceDirection _face) {
		Int3 placePos = Blocks::GetAdjacentBlockPos(_pos, _face);
		if (_face == PacketData::FaceDirection::Y_PLUS)
			_world.SetBlock(placePos, BLOCK_SIGN); //TODO: facing meta
		else
			_world.SetBlock(placePos, BLOCK_SIGN_WALL, _face);

		_stack->DecrementCount(1);
	};

	itemBehavior[MAP].onStartHolding = [](ItemStack* _stack, PlayerSession& _session) {
		GlobalLogger().debug << "Started holding a map!\n";
	};

	itemBehavior[MAP].whileHeld = [](ItemStack* _stack, PlayerSession& _session, Server& _server) {
		auto pos = _session.position.GetBlockPos();
		std::vector<Map::MarkerPlacement> markers = {
			// TODO: Evil fuck-ass offset magic???
			Map::MakeMarker(Map::Icon::WhiteArrow, _session.rotation.x,
			                Byte2{ static_cast<int8_t>(pos.x), static_cast<int8_t>(pos.z) })
		};
		std::vector<uint8_t> mapIcons;
		PlaceMarkers(mapIcons, markers);
		Packet::ItemData pkt;
		pkt.itemId = MAP;
		pkt.mapId = 0;
		pkt.data = mapIcons;
		pkt.Serialize(_session.stream);

		static int8_t x = 0;
		if (x >= INT8_MAX - 1)
			return;

		// TODO: Maps are 1:8 in B1.7.3
		// TODO: Maps were centered on the point they were crafted in B1.7.3
		auto world = _server.GetWorldForDimension(_session.dimension);
		std::vector<uint8_t> mapData;
		Map::InitGraphics(mapData, Byte2{ x, 0 });

		for (int z = 0; z < INT8_MAX; z++) {
			int8_t y = world->GetHeightValue(x, z);
			Int3 bpos{ x, y - 1, z };
			BlockType block = world->GetBlockId(bpos);
			Map::AppendPixel(mapData, (Blocks::blockProperties[block].material.mapColor.index << 2) | 2);
		}

		Packet::ItemData gfxPkt;
		gfxPkt.itemId = MAP;
		gfxPkt.mapId = 0;
		gfxPkt.data = mapData;
		gfxPkt.Serialize(_session.stream);
		x++;
	};

	itemBehavior[MAP].onStopHolding = [](ItemStack* _stack) {
		GlobalLogger().debug << "No longer holding a map!\n";
	};
};
}; // namespace Items