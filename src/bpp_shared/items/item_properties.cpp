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

bool IsValid(ItemId _id) {
	return ((_id >= Items::Id::SHOVEL_IRON && _id < Items::Id::MAX) ||
	        (_id >= Items::Id::RECORD_13 && _id < Items::Id::RECORD_MAX));
}

bool IsArmor(ItemId _id) {
	return (_id >= Items::HELMET_LEATHER && _id <= Items::BOOTS_GOLD);
}

bool IsHoe(ItemId _id) {
	return (_id >= Items::HOE_WOOD && _id <= Items::HOE_GOLD);
}

bool IsSword(ItemId _id) {
	return (_id == Items::SWORD_IRON || _id == Items::SWORD_WOOD || _id == Items::SWORD_STONE ||
	        _id == Items::SWORD_DIAMOND || _id == Items::SWORD_GOLD);
}

bool IsPickaxe(ItemId _id) {
	return (_id == Items::PICKAXE_IRON || _id == Items::PICKAXE_WOOD || _id == Items::PICKAXE_STONE ||
	        _id == Items::PICKAXE_DIAMOND || _id == Items::PICKAXE_GOLD);
}

bool IsAxe(ItemId _id) {
	return (_id == Items::AXE_IRON || _id == Items::AXE_WOOD || _id == Items::AXE_STONE || _id == Items::AXE_DIAMOND ||
	        _id == Items::AXE_GOLD);
}

bool IsShovel(ItemId _id) {
	return (_id == Items::SHOVEL_IRON || _id == Items::SHOVEL_WOOD || _id == Items::SHOVEL_STONE ||
	        _id == Items::SHOVEL_DIAMOND || _id == Items::SHOVEL_GOLD);
}

bool IsWeapon(ItemId _id) {
	return IsSword(_id) || _id == Items::BOW;
}

bool IsTool(ItemId _id) {
	return IsShovel(_id) || IsAxe(_id) || IsPickaxe(_id) || IsHoe(_id) || _id == Items::FLINT_AND_STEEL ||
	       _id == Items::FISHING_ROD || _id == Items::SHEARS;
}

bool IsThrowable(ItemId _id) {
	return (_id == Items::SNOWBALL || _id == Items::EGG);
}

bool IsBlock(ItemId _id) {
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

ItemDamage GetMaterialUses(ToolMaterial _material) {
	switch (_material) {
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

void harmTool(ItemStack* _stack) {
	_stack->data++;
	if (_stack->data >= toolProperties[_stack->id].max_uses) {
		_stack->decrementCount(1);
	}
}

void UseHoe(WorldManager& _world, ItemStack* _stack, Int3 _pos, PacketData::FaceDirection _face) {
	BlockType b = _world.getBlockId(_pos);
	if (b == BLOCK_GRASS || b == BLOCK_DIRT) {
		_world.setBlock(_pos, BLOCK_FARMLAND);
	}
	harmTool(_stack);
}

void UseFlintAndSteel(WorldManager& _world, ItemStack* _stack, Int3 _pos, PacketData::FaceDirection _face) {
	_pos = Blocks::getAdjacentBlockPos(_pos, _face);
	_world.setBlock(_pos, BLOCK_FIRE);
	harmTool(_stack);
}

void TestSetGoal(WorldManager& _world, ItemStack* _stack, Int3 _pos, PacketData::FaceDirection _face) {
	Int3 topPos = _pos;
	topPos.y += 1;
	_world.setBlock(topPos, BLOCK_AIR);
	std::cout << "lol!!" << std::endl;
	for (auto entity : _world.entityManager.entities) {
		std::cout << (int)entity->type << std::endl;
		if (entity->type == EntityType::CREEPER) {
			auto finder = std::static_pointer_cast<MobileEntity>(entity);
			std::cout << "Setting goal to" << topPos << std::endl;
			finder->setGoal(topPos);
		}
	}
}

ToolLevel MaterialToLevel(ToolMaterial _material) {
	switch (_material) {
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

int BaseToolDamage(ToolType _type) {
	switch (_type) {
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

EntityHealth CalculateDamage(ToolType _type, ToolLevel _level) {
	return BaseToolDamage(_type) + (int(_level) * 2);
}

void InflictDamage(Entity& _targetEntity, EntityHealth _damage) {
	//target_entity.health -= damage;
	return;
}

void AttackWithItem(Entity& _targetEntity, ItemStack* _stack) {
	EntityHealth damage = 1;
	if (toolProperties.contains(_stack->id))
		damage = CalculateDamage(toolProperties[_stack->id].type, MaterialToLevel(toolProperties[_stack->id].material));
	InflictDamage(_targetEntity, damage);
	GlobalLogger().info << "Dealt " << damage << " damage to " << _targetEntity.id << "!\n";
	harmTool(_stack);
}

void registerAll() {
	itemBehavior[Items::Id::HOE_WOOD] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_STONE] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_IRON] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_GOLD] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::HOE_DIAMOND] = ItemBehavior{ .onBlockUse = UseHoe };
	itemBehavior[Items::Id::FLINT_AND_STEEL] = ItemBehavior{ .onBlockStartMining = TestSetGoal,
		                                                     .onBlockUse = UseFlintAndSteel };

	// Tool Properties
	// Sword
	toolProperties[Items::Id::SWORD_WOOD] = ToolProperties{
		.type = ToolType::Sword,
		.material = ToolMaterial::Wooden,
	};
	toolProperties[Items::Id::SWORD_STONE] = ToolProperties{
		.type = ToolType::Sword,
		.material = ToolMaterial::Stone,
	};
	toolProperties[Items::Id::SWORD_IRON] = ToolProperties{
		.type = ToolType::Sword,
		.material = ToolMaterial::Iron,
	};
	toolProperties[Items::Id::SWORD_GOLD] = ToolProperties{
		.type = ToolType::Sword,
		.material = ToolMaterial::Gold,
	};
	toolProperties[Items::Id::SWORD_DIAMOND] = ToolProperties{
		.type = ToolType::Sword,
		.material = ToolMaterial::Diamond,
	};
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
	toolProperties[Items::Id::SHOVEL_WOOD] = ToolProperties{
		.type = ToolType::Shovel,
		.material = ToolMaterial::Wooden,
	};
	toolProperties[Items::Id::SHOVEL_STONE] = ToolProperties{
		.type = ToolType::Shovel,
		.material = ToolMaterial::Stone,
	};
	toolProperties[Items::Id::SHOVEL_IRON] = ToolProperties{
		.type = ToolType::Shovel,
		.material = ToolMaterial::Iron,
	};
	toolProperties[Items::Id::SHOVEL_GOLD] = ToolProperties{
		.type = ToolType::Shovel,
		.material = ToolMaterial::Gold,
	};
	toolProperties[Items::Id::SHOVEL_DIAMOND] = ToolProperties{
		.type = ToolType::Shovel,
		.material = ToolMaterial::Diamond,
	};
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
		toolProperty.second.max_uses = GetMaterialUses(toolProperty.second.material);
	}
	// Misc tools
	toolProperties[Items::Id::FLINT_AND_STEEL] = ToolProperties{ .type = ToolType::None,
		                                                         .material = ToolMaterial::None,
		                                                         .max_uses = DURABILITY_FLINT_AND_STEEL };
	toolProperties[Items::Id::SHEARS] = ToolProperties{ .type = ToolType::None,
		                                                .material = ToolMaterial::None,
		                                                .max_uses = DURABILITY_SHEARS };
	toolProperties[Items::Id::BOW] = ToolProperties{ .type = ToolType::None,
		                                             .material = ToolMaterial::None,
		                                             .max_uses = DURABILITY_BOW };
	toolProperties[Items::Id::FISHING_ROD] = ToolProperties{ .type = ToolType::None,
		                                                     .material = ToolMaterial::None,
		                                                     .max_uses = DURABILITY_FISHING_ROD };

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
		Int3 placePos = Blocks::getAdjacentBlockPos(_pos, _face);
		if (!Blocks::canSugarcaneSurviveAt(_world, placePos))
			return;

		_world.setBlock(placePos, BLOCK_SUGARCANE);
		_stack->decrementCount(1);
	};

	itemBehavior[SIGN].onBlockUse = [](WorldManager& _world, ItemStack* _stack, Int3 _pos, PacketData::FaceDirection _face) {
		Int3 placePos = Blocks::getAdjacentBlockPos(_pos, _face);
		if (_face == PacketData::FaceDirection::Y_PLUS)
			_world.setBlock(placePos, BLOCK_SIGN); //TODO: facing meta
		else
			_world.setBlock(placePos, BLOCK_SIGN_WALL, _face);

		_stack->decrementCount(1);
	};
};
}; // namespace Items