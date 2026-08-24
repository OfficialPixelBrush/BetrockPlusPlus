
/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "blocks.h"
#include "item_map.h"
#include "item_properties.h"
#include "items.h"
#include "server.h"
#include "tool_properties.h"

namespace Items {

void RegisterAll() {
	itemBehavior[Items::Id::HOE_WOOD] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_STONE] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_IRON] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_GOLD] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_DIAMOND] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::FLINT_AND_STEEL] = ItemBehavior{ .onBlockUse = UseFlintAndSteel };
	itemBehavior[Items::Id::BUCKET_WATER] = ItemBehavior{ .onBlockUse = UseWaterBucket };
	itemBehavior[Items::Id::BUCKET_LAVA] = ItemBehavior{ .onBlockUse = UseLavaBucket };
	itemBehavior[Items::Id::BUCKET] = ItemBehavior{ .onBlockUse = UseBucket };

	// Tool Properties
	// Sword
	toolProperties[Items::Id::SWORD_WOOD] = ToolProperties{ .type = ToolType::Sword,
		                                                    .material = ToolMaterial::Wooden,
		                                                    .canHarvest = CanShearsOrSwordHarvest,
		                                                    .howEffectiveAgainstBlock = SwordEffectiveness };
	toolProperties[Items::Id::SWORD_STONE] = ToolProperties{ .type = ToolType::Sword,
		                                                     .material = ToolMaterial::Stone,
		                                                     .canHarvest = CanShearsOrSwordHarvest,
		                                                     .howEffectiveAgainstBlock = SwordEffectiveness };
	toolProperties[Items::Id::SWORD_IRON] = ToolProperties{ .type = ToolType::Sword,
		                                                    .material = ToolMaterial::Iron,
		                                                    .canHarvest = CanShearsOrSwordHarvest,
		                                                    .howEffectiveAgainstBlock = SwordEffectiveness };
	toolProperties[Items::Id::SWORD_GOLD] = ToolProperties{ .type = ToolType::Sword,
		                                                    .material = ToolMaterial::Gold,
		                                                    .canHarvest = CanShearsOrSwordHarvest,
		                                                    .howEffectiveAgainstBlock = SwordEffectiveness };
	toolProperties[Items::Id::SWORD_DIAMOND] = ToolProperties{ .type = ToolType::Sword,
		                                                       .material = ToolMaterial::Diamond,
		                                                       .canHarvest = CanShearsOrSwordHarvest,
		                                                       .howEffectiveAgainstBlock = SwordEffectiveness };
	// Pickaxe
	toolProperties[Items::Id::PICKAXE_WOOD] = ToolProperties{ .type = ToolType::Pickaxe,
		                                                      .material = ToolMaterial::Wooden,
		                                                      .canHarvest = CanPickaxeHarvest,
		                                                      .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::PICKAXE_STONE] = ToolProperties{ .type = ToolType::Pickaxe,
		                                                       .material = ToolMaterial::Stone,
		                                                       .canHarvest = CanPickaxeHarvest,
		                                                       .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::PICKAXE_IRON] = ToolProperties{ .type = ToolType::Pickaxe,
		                                                      .material = ToolMaterial::Iron,
		                                                      .canHarvest = CanPickaxeHarvest,
		                                                      .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::PICKAXE_GOLD] = ToolProperties{ .type = ToolType::Pickaxe,
		                                                      .material = ToolMaterial::Gold,
		                                                      .canHarvest = CanPickaxeHarvest,
		                                                      .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::PICKAXE_DIAMOND] = ToolProperties{ .type = ToolType::Pickaxe,
		                                                         .material = ToolMaterial::Diamond,
		                                                         .canHarvest = CanPickaxeHarvest,
		                                                         .howEffectiveAgainstBlock = ToolEffectiveness };
	// Axe
	toolProperties[Items::Id::AXE_WOOD] = ToolProperties{ .type = ToolType::Axe,
		                                                  .material = ToolMaterial::Wooden,
		                                                  .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::AXE_STONE] = ToolProperties{ .type = ToolType::Axe,
		                                                   .material = ToolMaterial::Stone,
		                                                   .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::AXE_IRON] = ToolProperties{ .type = ToolType::Axe,
		                                                  .material = ToolMaterial::Iron,
		                                                  .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::AXE_GOLD] = ToolProperties{ .type = ToolType::Axe,
		                                                  .material = ToolMaterial::Gold,
		                                                  .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::AXE_DIAMOND] = ToolProperties{ .type = ToolType::Axe,
		                                                     .material = ToolMaterial::Diamond,
		                                                     .howEffectiveAgainstBlock = ToolEffectiveness };
	// Shovel
	toolProperties[Items::Id::SHOVEL_WOOD] = ToolProperties{ .type = ToolType::Shovel,
		                                                     .material = ToolMaterial::Wooden,
		                                                     .canHarvest = CanShovelHarvest,
		                                                     .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::SHOVEL_STONE] = ToolProperties{ .type = ToolType::Shovel,
		                                                      .material = ToolMaterial::Stone,
		                                                      .canHarvest = CanShovelHarvest,
		                                                      .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::SHOVEL_IRON] = ToolProperties{ .type = ToolType::Shovel,
		                                                     .material = ToolMaterial::Iron,
		                                                     .canHarvest = CanShovelHarvest,
		                                                     .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::SHOVEL_GOLD] = ToolProperties{ .type = ToolType::Shovel,
		                                                     .material = ToolMaterial::Gold,
		                                                     .canHarvest = CanShovelHarvest,
		                                                     .howEffectiveAgainstBlock = ToolEffectiveness };
	toolProperties[Items::Id::SHOVEL_DIAMOND] = ToolProperties{ .type = ToolType::Shovel,
		                                                        .material = ToolMaterial::Diamond,
		                                                        .canHarvest = CanShovelHarvest,
		                                                        .howEffectiveAgainstBlock = ToolEffectiveness };

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
		                                                .canHarvest = CanShearsOrSwordHarvest,
		                                                .howEffectiveAgainstBlock = ShearsEffectiveness };
	toolProperties[Items::Id::BOW] = ToolProperties{ .type = ToolType::None,
		                                             .material = ToolMaterial::None,
		                                             .maxUses = DURABILITY_BOW };
	toolProperties[Items::Id::FISHING_ROD] = ToolProperties{ .type = ToolType::None,
		                                                     .material = ToolMaterial::None,
		                                                     .maxUses = DURABILITY_FISHING_ROD };

	// Armor Leather
	toolProperties[Items::Id::HELMET_LEATHER] = ToolProperties{ .maxUses = DURABILITY_HELMET_LEATHER };
	toolProperties[Items::Id::CHESTPLATE_LEATHER] = ToolProperties{ .maxUses = DURABILITY_CHEST_LEATHER };
	toolProperties[Items::Id::LEGGINGS_LEATHER] = ToolProperties{ .maxUses = DURABILITY_LEGS_LEATHER };
	toolProperties[Items::Id::BOOTS_LEATHER] = ToolProperties{ .maxUses = DURABILITY_BOOTS_LEATHER };

	// Armor Chainmail
	toolProperties[Items::Id::HELMET_CHAINMAIL] = ToolProperties{ .maxUses = DURABILITY_HELMET_CHAINMAIL };
	toolProperties[Items::Id::CHESTPLATE_CHAINMAIL] = ToolProperties{ .maxUses = DURABILITY_CHEST_CHAINMAIL };
	toolProperties[Items::Id::LEGGINGS_CHAINMAIL] = ToolProperties{ .maxUses = DURABILITY_LEGS_CHAINMAIL };
	toolProperties[Items::Id::BOOTS_CHAINMAIL] = ToolProperties{ .maxUses = DURABILITY_BOOTS_CHAINMAIL };

	// Armor Iron
	toolProperties[Items::Id::HELMET_IRON] = ToolProperties{ .maxUses = DURABILITY_HELMET_IRON };
	toolProperties[Items::Id::CHESTPLATE_IRON] = ToolProperties{ .maxUses = DURABILITY_CHEST_IRON };
	toolProperties[Items::Id::LEGGINGS_IRON] = ToolProperties{ .maxUses = DURABILITY_LEGS_IRON };
	toolProperties[Items::Id::BOOTS_IRON] = ToolProperties{ .maxUses = DURABILITY_BOOTS_IRON };

	// Armor Diamond
	toolProperties[Items::Id::HELMET_DIAMOND] = ToolProperties{ .maxUses = DURABILITY_HELMET_DIAMOND };
	toolProperties[Items::Id::CHESTPLATE_DIAMOND] = ToolProperties{ .maxUses = DURABILITY_CHEST_DIAMOND };
	toolProperties[Items::Id::LEGGINGS_DIAMOND] = ToolProperties{ .maxUses = DURABILITY_LEGS_DIAMOND };
	toolProperties[Items::Id::BOOTS_DIAMOND] = ToolProperties{ .maxUses = DURABILITY_BOOTS_DIAMOND };

	// Armor Gold
	toolProperties[Items::Id::HELMET_GOLD] = ToolProperties{ .maxUses = DURABILITY_HELMET_GOLD };
	toolProperties[Items::Id::CHESTPLATE_GOLD] = ToolProperties{ .maxUses = DURABILITY_CHEST_GOLD };
	toolProperties[Items::Id::LEGGINGS_GOLD] = ToolProperties{ .maxUses = DURABILITY_LEGS_GOLD };
	toolProperties[Items::Id::BOOTS_GOLD] = ToolProperties{ .maxUses = DURABILITY_BOOTS_GOLD };

	// Tool behaviors — iterate registered tools (toolBehavior starts empty)
	for (const auto& [itemId, props] : toolProperties) {
		ToolBehavior behavior{};
		switch (props.type) {
		case ToolType::Sword:
			behavior.onBlockFinishMining = OnToolFinishMining;
			behavior.onEntityAttack = AttackWithItem;
			break;
		case ToolType::Pickaxe:
		case ToolType::Axe:
		case ToolType::Shovel:
			behavior.onBlockFinishMining = OnToolFinishMining;
			behavior.onEntityAttack = AttackWithItem;
			break;
		case ToolType::Hoe:
			itemBehavior[itemId] = ItemBehavior{ .onBlockUse = UseHoe };
			break;
		case ToolType::None:
			if (itemId == Items::Id::SHEARS)
				behavior.onBlockFinishMining = OnToolFinishMining;
			break;
		}
		toolBehavior[itemId] = behavior;
	}
	toolBehavior[Items::Id::SHEARS] = ToolBehavior{ .onEntityUse = UseShears };

	// Item behaviors
	itemBehavior[APPLE].onUse = EatFood;
	itemBehavior[BREAD].onUse = EatFood;
	itemBehavior[PORKCHOP].onUse = EatFood;
	itemBehavior[PORKCHOP_COOKED].onUse = EatFood;
	itemBehavior[APPLE_GOLDEN].onUse = EatFood;
	itemBehavior[COOKIE].onUse = EatFood;
	itemBehavior[FISH].onUse = EatFood;
	itemBehavior[FISH_COOKED].onUse = EatFood;
	itemBehavior[MUSHROOM_STEW].onUse = EatFood;

	itemBehavior[SUGARCANE].onBlockUse = [](WorldManager& _world, ItemStack* _stack, Int3 _pos, Entity& _user,
	                                        PacketData::FaceDirection _face) {
		Int3 placePos = Blocks::GetAdjacentBlockPos(_pos, _face);
		if (!Blocks::CanSugarcaneSurviveAt(_world, placePos))
			return;

		_world.SetBlock(placePos, BLOCK_SUGARCANE);
		_stack->DecrementCount(1);
	};

	itemBehavior[SEEDS_WHEAT].onBlockUse = [](WorldManager& _world, ItemStack* _stack, Int3 _pos, Entity& _user,
	                                        PacketData::FaceDirection _face) {
		Int3 placePos = Blocks::GetAdjacentBlockPos(_pos, _face);
		if (!Blocks::CanCropsSurviveAt(_world, placePos))
			return;

		_world.SetBlock(placePos, BLOCK_CROP_WHEAT);
		_stack->DecrementCount(1);
	};

	itemBehavior[SIGN].onBlockUse = [](WorldManager& _world, ItemStack* _stack, Int3 _pos, Entity& _user,
	                                   PacketData::FaceDirection _face) {
		Int3 placePos = Blocks::GetAdjacentBlockPos(_pos, _face);
		if (_face == PacketData::FaceDirection::Y_PLUS)
			_world.SetBlock(placePos, BLOCK_SIGN); //TODO: facing meta
		else
			_world.SetBlock(placePos, BLOCK_SIGN_WALL, _face);

		_stack->DecrementCount(1);
	};

	auto onDoorPlace = [](WorldManager& _world, ItemStack* _stack, Int3 _pos, Entity& _user,
	                      PacketData::FaceDirection _face) {
		BlockType targetBlockType;
		targetBlockType = _stack->id == DOOR_WOOD ? BLOCK_DOOR_WOOD : BLOCK_DOOR_IRON;
		Int3 placePosition = Blocks::GetAdjacentBlockPos(_pos, _face);
		if (Blocks::blockBehaviors[targetBlockType].onBlockPlaced(_world, placePosition, _user, _face, targetBlockType,
		                                                          0))
			_stack->DecrementCount(1);
	};

	itemBehavior[DOOR_WOOD].onBlockUse = onDoorPlace;
	itemBehavior[DOOR_IRON].onBlockUse = onDoorPlace;

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
			int8_t northY = world->GetHeightValue(x, z - 1);
			int8_t y = world->GetHeightValue(x, z);
			Int3 bpos{ x, y - 1, z };
			BlockType block = world->GetBlockId(bpos);
			int8_t brightness = 1;
			if (northY > y)
				brightness--;
			if (northY < y)
				brightness++;
			Map::AppendPixel(mapData, (Blocks::blockProperties[block].material.mapColor.index << 2) | brightness);
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