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
#include "entities/entity.h"
#include "entities/entity_mobile.h"
#include "enums/items.h"
#include "inventory/item_stack.h"
#include "logger.h"
#include "packet_data.h"
#include "world/world.h"

namespace Items {

// Tool material durability
static constexpr ItemDamage DURABILITY_WOOD = 59;
static constexpr ItemDamage DURABILITY_STONE = 131;
static constexpr ItemDamage DURABILITY_IRON = 250;
static constexpr ItemDamage DURABILITY_DIAMOND = 1561;
static constexpr ItemDamage DURABILITY_GOLD = 32;

// Armor max damage = maxDamageArray[slot] * 3 << material
// material: leather=0, chain/gold=1, iron=2, diamond=3
// slot: helmet=0(x11), chest=1(x16), legs=2(x15), boots=3(x13)
static constexpr ItemDamage DURABILITY_HELMET_LEATHER = 11 * 3 * 1; // 33
static constexpr ItemDamage DURABILITY_CHEST_LEATHER = 16 * 3 * 1;  // 48
static constexpr ItemDamage DURABILITY_LEGS_LEATHER = 15 * 3 * 1;   // 45
static constexpr ItemDamage DURABILITY_BOOTS_LEATHER = 13 * 3 * 1;  // 39

static constexpr ItemDamage DURABILITY_HELMET_CHAINMAIL = 11 * 3 * 2; // 66
static constexpr ItemDamage DURABILITY_CHEST_CHAINMAIL = 16 * 3 * 2;  // 96
static constexpr ItemDamage DURABILITY_LEGS_CHAINMAIL = 15 * 3 * 2;   // 90
static constexpr ItemDamage DURABILITY_BOOTS_CHAINMAIL = 13 * 3 * 2;  // 78

static constexpr ItemDamage DURABILITY_HELMET_IRON = 11 * 3 * 4; // 132
static constexpr ItemDamage DURABILITY_CHEST_IRON = 16 * 3 * 4;  // 192
static constexpr ItemDamage DURABILITY_LEGS_IRON = 15 * 3 * 4;   // 180
static constexpr ItemDamage DURABILITY_BOOTS_IRON = 13 * 3 * 4;  // 156

static constexpr ItemDamage DURABILITY_HELMET_DIAMOND = 11 * 3 * 8; // 264
static constexpr ItemDamage DURABILITY_CHEST_DIAMOND = 16 * 3 * 8;  // 384
static constexpr ItemDamage DURABILITY_LEGS_DIAMOND = 15 * 3 * 8;   // 360
static constexpr ItemDamage DURABILITY_BOOTS_DIAMOND = 13 * 3 * 8;  // 312

static constexpr ItemDamage DURABILITY_HELMET_GOLD = 11 * 3 * 2; // 66
static constexpr ItemDamage DURABILITY_CHEST_GOLD = 16 * 3 * 2;  // 96
static constexpr ItemDamage DURABILITY_LEGS_GOLD = 15 * 3 * 2;   // 90
static constexpr ItemDamage DURABILITY_BOOTS_GOLD = 13 * 3 * 2;  // 78

static constexpr ItemDamage DURABILITY_FISHING_ROD = 64;
static constexpr ItemDamage DURABILITY_FLINT_AND_STEEL = 64;
static constexpr ItemDamage DURABILITY_SHEARS = 238;
static constexpr ItemDamage DURABILITY_BOW = 384;

// Global table definitions; declared extern in the header
std::unordered_map<ItemId, ItemBehavior> itemBehavior = {};
std::unordered_map<ItemId, ItemProperties> itemProperties = {};
std::unordered_map<ItemId, ToolProperties> toolProperties = {};

bool IsValid(ItemId id) {
	return ((id >= Items::Id::SHOVEL_IRON && id < Items::Id::MAX) ||
	        (id >= Items::Id::RECORD_13 && id < Items::Id::RECORD_MAX));
}

bool IsArmor(ItemId id) {
	return (id >= Items::HELMET_LEATHER && id <= Items::BOOTS_GOLD);
}

bool IsHoe(ItemId id) {
	return (id >= Items::HOE_WOOD && id <= Items::HOE_GOLD);
}

bool IsSword(ItemId id) {
	return (id == Items::SWORD_IRON || id == Items::SWORD_WOOD || id == Items::SWORD_STONE ||
	        id == Items::SWORD_DIAMOND || id == Items::SWORD_GOLD);
}

bool IsPickaxe(ItemId id) {
	return (id == Items::PICKAXE_IRON || id == Items::PICKAXE_WOOD || id == Items::PICKAXE_STONE ||
	        id == Items::PICKAXE_DIAMOND || id == Items::PICKAXE_GOLD);
}

bool IsAxe(ItemId id) {
	return (id == Items::AXE_IRON || id == Items::AXE_WOOD || id == Items::AXE_STONE || id == Items::AXE_DIAMOND ||
	        id == Items::AXE_GOLD);
}

bool IsShovel(ItemId id) {
	return (id == Items::SHOVEL_IRON || id == Items::SHOVEL_WOOD || id == Items::SHOVEL_STONE ||
	        id == Items::SHOVEL_DIAMOND || id == Items::SHOVEL_GOLD);
}

bool IsWeapon(ItemId id) {
	return IsSword(id) || id == Items::BOW;
}

bool IsTool(ItemId id) {
	return IsShovel(id) || IsAxe(id) || IsPickaxe(id) || IsHoe(id) || id == Items::FLINT_AND_STEEL ||
	       id == Items::FISHING_ROD || id == Items::SHEARS;
}

bool IsThrowable(ItemId id) {
	return (id == Items::SNOWBALL || id == Items::EGG);
}

bool IsBlock(ItemId id) {
	return (id > 0 && id <= Items::THRESHOLD);
}

bool IsStackable(ItemId id) {
	return Items::GetMaxStack(id) > 1;
}

int32_t GetMaxStack(ItemId id) {
	// Stack size 1
	switch (id) {
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
	if (IsTool(id) || IsWeapon(id) || IsArmor(id))
		return 1;

	// Stack size 16
	if (id == Items::SNOWBALL || id == Items::EGG)
		return 16;

	if (id == Items::COOKIE)
		return 8;

	// Item, ItemCoal, ItemSeeds, ItemRedstone, ItemDye, ItemPainting,
	// ItemReed (sugarcane & repeater item), ItemRecord (never reached above),
	// all blocks, and any resource item not listed above.
	return Items::STACK_MAX;
}

ItemDamage GetMaterialUses(ToolMaterial material) {
	switch (material) {
	case ToolMaterial::Wooden:
		return DURABILITY_WOOD;
	case ToolMaterial::Gold:
		return DURABILITY_GOLD;
	case ToolMaterial::Stone:
		return DURABILITY_STONE;
	case ToolMaterial::Iron:
		return DURABILITY_IRON;
	case ToolMaterial::Diamond:
		return DURABILITY_DIAMOND;
	default:
		return -1;
	}
}

void harmTool(ItemStack* stack) {
	stack->m_data++;
	if (stack->m_data >= toolProperties[stack->m_id].m_max_uses) {
		stack->decrementCount(1);
	}
}

void useHoe(WorldManager& world, ItemStack* stack, Int3 pos, PacketData::FaceDirection face) {
	BlockType b = world.getBlockId(pos);
	if (b == BLOCK_GRASS || b == BLOCK_DIRT) {
		world.setBlock(pos, BLOCK_FARMLAND);
	}
	harmTool(stack);
}

void useFlintAndSteel(WorldManager& world, ItemStack* stack, Int3 pos, PacketData::FaceDirection face) {
	pos = Blocks::getAdjacentBlockPos(pos, face);
	world.setBlock(pos, BLOCK_FIRE);
	harmTool(stack);
}

void testSetGoal(WorldManager& world, ItemStack* stack, Int3 pos, PacketData::FaceDirection face) {
	Int3 topPos = pos;
	topPos.m_y += 1;
	world.setBlock(topPos, BLOCK_AIR);
	std::cout << "lol!!" << std::endl;
	for (auto entity : world.m_entityManager.m_entities) {
		std::cout << (int)entity->m_type << std::endl;
		if (entity->m_type == EntityType::CREEPER) {
			auto finder = std::static_pointer_cast<MobileEntity>(entity);
			std::cout << "Setting goal to" << topPos << std::endl;
			finder->setGoal(topPos);
		}
	}
}

ToolLevel materialToLevel(ToolMaterial material) {
	switch (material) {
	case ToolMaterial::None:
		return ToolLevel::None;
	case ToolMaterial::Wooden:
		return ToolLevel::WoodenOrGold;
	case ToolMaterial::Gold:
		return ToolLevel::WoodenOrGold;
	case ToolMaterial::Stone:
		return ToolLevel::Stone;
	case ToolMaterial::Iron:
		return ToolLevel::Iron;
	case ToolMaterial::Diamond:
		return ToolLevel::Diamond;
	}
	return ToolLevel::None;
}

int baseToolDamage(ToolType type) {
	switch (type) {
	case ToolType::Sword:
		return 4;
	case ToolType::Axe:
		return 3;
	case ToolType::Pickaxe:
		return 2;
	case ToolType::Shovel:
	case ToolType::Hoe:
		return 1;
	case ToolType::None:
		return 0;
	}
	// Default
	return 0;
}

EntityHealth calculateDamage(ToolType type, ToolLevel level) {
	return baseToolDamage(type) + (int(level) * 2);
}

void inflictDamage(Entity& target_entity, EntityHealth damage) {
	//target_entity.health -= damage;
	return;
}

void attackWithItem(Entity& target_entity, ItemStack* stack) {
	EntityHealth damage = 1;
	if (toolProperties.contains(stack->m_id))
		damage = calculateDamage(toolProperties[stack->m_id].m_type, materialToLevel(toolProperties[stack->m_id].m_material));
	inflictDamage(target_entity, damage);
	GlobalLogger().m_info << "Dealt " << damage << " damage to " << target_entity.m_id << "!\n";
	harmTool(stack);
}

void registerAll() {
	itemBehavior[Items::Id::HOE_WOOD] = ItemBehavior{ .m_onBlockUse = useHoe };
	itemBehavior[Items::Id::HOE_STONE] = ItemBehavior{ .m_onBlockUse = useHoe };
	itemBehavior[Items::Id::HOE_IRON] = ItemBehavior{ .m_onBlockUse = useHoe };
	itemBehavior[Items::Id::HOE_GOLD] = ItemBehavior{ .m_onBlockUse = useHoe };
	itemBehavior[Items::Id::HOE_DIAMOND] = ItemBehavior{ .m_onBlockUse = useHoe };
	itemBehavior[Items::Id::FLINT_AND_STEEL] = ItemBehavior{ .m_onBlockStartMining = testSetGoal,
		                                                     .m_onBlockUse = useFlintAndSteel };

	// Tool Properties
	// Sword
	toolProperties[Items::Id::SWORD_WOOD] = ToolProperties{
		.m_type = ToolType::Sword,
		.m_material = ToolMaterial::Wooden,
	};
	toolProperties[Items::Id::SWORD_STONE] = ToolProperties{
		.m_type = ToolType::Sword,
		.m_material = ToolMaterial::Stone,
	};
	toolProperties[Items::Id::SWORD_IRON] = ToolProperties{
		.m_type = ToolType::Sword,
		.m_material = ToolMaterial::Iron,
	};
	toolProperties[Items::Id::SWORD_GOLD] = ToolProperties{
		.m_type = ToolType::Sword,
		.m_material = ToolMaterial::Gold,
	};
	toolProperties[Items::Id::SWORD_DIAMOND] = ToolProperties{
		.m_type = ToolType::Sword,
		.m_material = ToolMaterial::Diamond,
	};
	// Pickaxe
	toolProperties[Items::Id::PICKAXE_WOOD] = ToolProperties{
		.m_type = ToolType::Pickaxe,
		.m_material = ToolMaterial::Wooden,
	};
	toolProperties[Items::Id::PICKAXE_STONE] = ToolProperties{
		.m_type = ToolType::Pickaxe,
		.m_material = ToolMaterial::Stone,
	};
	toolProperties[Items::Id::PICKAXE_IRON] = ToolProperties{
		.m_type = ToolType::Pickaxe,
		.m_material = ToolMaterial::Iron,
	};
	toolProperties[Items::Id::PICKAXE_GOLD] = ToolProperties{
		.m_type = ToolType::Pickaxe,
		.m_material = ToolMaterial::Gold,
	};
	toolProperties[Items::Id::PICKAXE_DIAMOND] = ToolProperties{
		.m_type = ToolType::Pickaxe,
		.m_material = ToolMaterial::Diamond,
	};
	// Axe
	toolProperties[Items::Id::AXE_WOOD] = ToolProperties{
		.m_type = ToolType::Axe,
		.m_material = ToolMaterial::Wooden,
	};
	toolProperties[Items::Id::AXE_STONE] = ToolProperties{
		.m_type = ToolType::Axe,
		.m_material = ToolMaterial::Stone,
	};
	toolProperties[Items::Id::AXE_IRON] = ToolProperties{
		.m_type = ToolType::Axe,
		.m_material = ToolMaterial::Iron,
	};
	toolProperties[Items::Id::AXE_GOLD] = ToolProperties{
		.m_type = ToolType::Axe,
		.m_material = ToolMaterial::Gold,
	};
	toolProperties[Items::Id::AXE_DIAMOND] = ToolProperties{
		.m_type = ToolType::Axe,
		.m_material = ToolMaterial::Diamond,
	};
	// Shovel
	toolProperties[Items::Id::SHOVEL_WOOD] = ToolProperties{
		.m_type = ToolType::Shovel,
		.m_material = ToolMaterial::Wooden,
	};
	toolProperties[Items::Id::SHOVEL_STONE] = ToolProperties{
		.m_type = ToolType::Shovel,
		.m_material = ToolMaterial::Stone,
	};
	toolProperties[Items::Id::SHOVEL_IRON] = ToolProperties{
		.m_type = ToolType::Shovel,
		.m_material = ToolMaterial::Iron,
	};
	toolProperties[Items::Id::SHOVEL_GOLD] = ToolProperties{
		.m_type = ToolType::Shovel,
		.m_material = ToolMaterial::Gold,
	};
	toolProperties[Items::Id::SHOVEL_DIAMOND] = ToolProperties{
		.m_type = ToolType::Shovel,
		.m_material = ToolMaterial::Diamond,
	};
	// Hoe
	toolProperties[Items::Id::HOE_WOOD] = ToolProperties{
		.m_type = ToolType::Hoe,
		.m_material = ToolMaterial::Wooden,
	};
	toolProperties[Items::Id::HOE_STONE] = ToolProperties{
		.m_type = ToolType::Hoe,
		.m_material = ToolMaterial::Stone,
	};
	toolProperties[Items::Id::HOE_IRON] = ToolProperties{
		.m_type = ToolType::Hoe,
		.m_material = ToolMaterial::Iron,
	};
	toolProperties[Items::Id::HOE_GOLD] = ToolProperties{
		.m_type = ToolType::Hoe,
		.m_material = ToolMaterial::Gold,
	};
	toolProperties[Items::Id::HOE_DIAMOND] = ToolProperties{
		.m_type = ToolType::Hoe,
		.m_material = ToolMaterial::Diamond,
	};
	// Apply max uses based on material
	for (auto& toolProperty : toolProperties) {
		toolProperty.second.m_max_uses = GetMaterialUses(toolProperty.second.m_material);
	}
	// Misc tools
	toolProperties[Items::Id::FLINT_AND_STEEL] = ToolProperties{ .m_type = ToolType::None,
		                                                         .m_material = ToolMaterial::None,
		                                                         .m_max_uses = DURABILITY_FLINT_AND_STEEL };
	toolProperties[Items::Id::SHEARS] = ToolProperties{ .m_type = ToolType::None,
		                                                .m_material = ToolMaterial::None,
		                                                .m_max_uses = DURABILITY_SHEARS };
	toolProperties[Items::Id::BOW] = ToolProperties{ .m_type = ToolType::None,
		                                             .m_material = ToolMaterial::None,
		                                             .m_max_uses = DURABILITY_BOW };
	toolProperties[Items::Id::FISHING_ROD] = ToolProperties{ .m_type = ToolType::None,
		                                                     .m_material = ToolMaterial::None,
		                                                     .m_max_uses = DURABILITY_FISHING_ROD };

	// Item behaviors
	for (auto& toolProperty : toolProperties) {
		// Apply sword behavior to all swords
		switch (toolProperty.second.m_type) {
		case ToolType::Sword:
			itemBehavior[toolProperty.first] = ItemBehavior{ .m_onEntityAttack = attackWithItem };
			continue;
		case ToolType::Pickaxe:
			continue;
		case ToolType::Axe:
			continue;
		case ToolType::Shovel:
			continue;
		case ToolType::Hoe:
			itemBehavior[toolProperty.first] = ItemBehavior{ .m_onBlockUse = useHoe };
			continue;
		default:
			continue;
		}
	}

	itemBehavior[SUGARCANE].m_onBlockUse = [](WorldManager& world, ItemStack* stack, Int3 pos,
	                                        PacketData::FaceDirection face) {
		Int3 placePos = Blocks::getAdjacentBlockPos(pos, face);
		if (!Blocks::canSugarcaneSurviveAt(world, placePos))
			return;

		world.setBlock(placePos, BLOCK_SUGARCANE);
		stack->decrementCount(1);
	};

	itemBehavior[SIGN].m_onBlockUse = [](WorldManager& world, ItemStack* stack, Int3 pos, PacketData::FaceDirection face) {
		Int3 placePos = Blocks::getAdjacentBlockPos(pos, face);
		if (face == PacketData::FaceDirection::Y_PLUS)
			world.setBlock(placePos, BLOCK_SIGN); //TODO: facing meta
		else
			world.setBlock(placePos, BLOCK_SIGN_WALL, face);

		stack->decrementCount(1);
	};
};
}; // namespace Items