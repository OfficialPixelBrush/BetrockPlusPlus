/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "tool_properties.h"
#include "blocks/block_properties.h"
#include "entities/entity.h"
#include "entities/entity_mobile.h"
#include "logger.h"

namespace Items {
std::unordered_map<ItemId, ToolProperties> toolProperties = {};

constexpr bool IsHoe(ItemId _id) {
	return (_id >= Items::HOE_WOOD && _id <= Items::HOE_GOLD);
}

constexpr bool IsSword(ItemId _id) {
	return (_id == Items::SWORD_IRON || _id == Items::SWORD_WOOD || _id == Items::SWORD_STONE ||
	        _id == Items::SWORD_DIAMOND || _id == Items::SWORD_GOLD);
}

constexpr bool IsPickaxe(ItemId _id) {
	return (_id == Items::PICKAXE_IRON || _id == Items::PICKAXE_WOOD || _id == Items::PICKAXE_STONE ||
	        _id == Items::PICKAXE_DIAMOND || _id == Items::PICKAXE_GOLD);
}

constexpr bool IsAxe(ItemId _id) {
	return (_id == Items::AXE_IRON || _id == Items::AXE_WOOD || _id == Items::AXE_STONE || _id == Items::AXE_DIAMOND ||
	        _id == Items::AXE_GOLD);
}

constexpr bool IsShovel(ItemId _id) {
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

constexpr ToolLevel MaterialToLevel(ToolMaterial _material) {
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

constexpr float MaterialToEfficiency(ToolMaterial _material) {
	switch (_material) {
	case ToolMaterial::None:
		return 1.0f;
	case ToolMaterial::Wooden:
		return 2.0f;
	case ToolMaterial::Gold:
		return 12.0f;
	case ToolMaterial::Stone:
		return 4.0f;
	case ToolMaterial::Iron:
		return 6.0f;
	case ToolMaterial::Diamond:
		return 8.0f;
	}
	return 1.0f;
}

constexpr int BaseToolDamage(ToolType _type) {
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

constexpr bool IsEffective(ToolType _type, BlockType _block) {
	switch (_type) {
	case ToolType::Pickaxe:
		return (_block == BLOCK_COBBLESTONE || _block == BLOCK_DOUBLE_SLAB || _block == BLOCK_SLAB ||
		        _block == BLOCK_STONE || _block == BLOCK_SANDSTONE || _block == BLOCK_COBBLESTONE_MOSSY ||
		        _block == BLOCK_ORE_IRON || _block == BLOCK_IRON || _block == BLOCK_ORE_COAL ||
		        _block == BLOCK_ORE_GOLD || _block == BLOCK_ORE_DIAMOND || _block == BLOCK_DIAMOND ||
		        _block == BLOCK_ICE || _block == BLOCK_NETHERRACK || _block == BLOCK_ORE_LAPIS_LAZULI ||
		        _block == BLOCK_LAPIS_LAZULI);
	case ToolType::Axe:
		return (_block == BLOCK_PLANKS || _block == BLOCK_BOOKSHELF || _block == BLOCK_LOG || _block == BLOCK_CHEST);
	default:
		return false;
	}
}

bool CanPickaxeHarvest(ToolLevel _level, BlockType _block) {
	if (_block == BLOCK_OBSIDIAN)
		return (_level == ToolLevel::Diamond);
	if (_block == BLOCK_DIAMOND || _block == BLOCK_ORE_DIAMOND)
		return (_level >= ToolLevel::Iron);
	if (_block == BLOCK_GOLD || _block == BLOCK_ORE_GOLD)
		return (_level >= ToolLevel::Iron);
	if (_block == BLOCK_IRON || _block == BLOCK_ORE_IRON)
		return (_level >= ToolLevel::Stone);
	if (_block == BLOCK_LAPIS_LAZULI || _block == BLOCK_ORE_LAPIS_LAZULI)
		return (_level >= ToolLevel::Stone);
	if (_block == BLOCK_ORE_REDSTONE_OFF || _block == BLOCK_ORE_REDSTONE_ON)
		return (_level >= ToolLevel::Iron);
	auto mat = Blocks::blockProperties[_block].material;
	if (mat != Material::Rock()) {
		return (mat == Material::Iron());
	}
	return true;
}

bool CanShovelHarvest(ToolLevel _level, BlockType _block) {
	return (_block == BLOCK_SNOW_LAYER || _block == BLOCK_SNOW);
}

bool CanShearsOrSwordHarvest(ToolLevel _level, BlockType _block) {
	return (_block == BLOCK_COBWEB);
}

float ShearsEffectiveness(ItemStack* _stack, BlockType _block) {
	switch (_block) {
	case BLOCK_COBWEB:
	case BLOCK_LEAVES:
		return 15.0f;
	case BLOCK_WOOL:
		return 5.0f;
	default:
		return 1.0f;
	}
}

float BlockDamagePerTickWithTool(ItemStack* _stack, BlockType _targetBlock, bool _reducedSpeed) {
	auto fn = Items::toolProperties[_stack->id].howEffectiveAgainstBlock;
	// Non-tool or tool without defined effectiveness
	if (!fn)
		return 1.0f;
	float efficiency = fn(_stack, _targetBlock);
	// Used if in water or not on ground
	if (_reducedSpeed)
		efficiency /= 5.0f;
	// Check if tool is effective against block
	return efficiency;
}

// Block "damage" is between 0.0 and 1.0
// A block is broken when it hits 1.0
float BlockDamagePerTick(ItemStack* _stack, BlockType _targetBlock, bool _reducedSpeed) {
	float hardness = Blocks::blockProperties[_targetBlock].hardness;
	if (hardness < 0.0f)
		return 0.0f;
	ToolLevel level = MaterialToLevel(toolProperties[_stack->id].material);
	// Using the right tool
	if (Items::toolProperties[_stack->id].canHarvest &&
	    Items::toolProperties[_stack->id].canHarvest(level, _targetBlock)) {
		return BlockDamagePerTickWithTool(_stack, _targetBlock, _reducedSpeed);
	}
	// Using an ineffective tool
	return 1.0f / hardness / 100.0f;
}

void AttackWithItem(Entity& _targetEntity, ItemStack* _stack) {
	EntityHealth damage = 1;
	if (toolProperties.contains(_stack->id))
		damage = CalculateDamage(toolProperties[_stack->id].type, MaterialToLevel(toolProperties[_stack->id].material));
	InflictDamage(_targetEntity, damage);
	GlobalLogger().info << "Dealt " << damage << " damage to " << _targetEntity.id << "!\n";
	HarmTool(_stack);
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

void HarmTool(ItemStack* _stack) {
	_stack->data++;
	if (_stack->data >= toolProperties[_stack->id].maxUses) {
		_stack->DecrementCount(1);
	}
}

void UseHoe(WorldManager& _world, ItemStack* _stack, Int3 _pos, PacketData::FaceDirection _face) {
	BlockType b = _world.GetBlockId(_pos);
	if (b == BLOCK_GRASS || b == BLOCK_DIRT) {
		_world.SetBlock(_pos, BLOCK_FARMLAND);
	}
	HarmTool(_stack);
}

void UseFlintAndSteel(WorldManager& _world, ItemStack* _stack, Int3 _pos, PacketData::FaceDirection _face) {
	_pos = Blocks::GetAdjacentBlockPos(_pos, _face);
	_world.SetBlock(_pos, BLOCK_FIRE);
	HarmTool(_stack);
}

void TestSetGoal(WorldManager& _world, ItemStack* _stack, Int3 _pos, PacketData::FaceDirection _face) {
	Int3 topPos = _pos;
	topPos.y += 1;
	_world.SetBlock(topPos, BLOCK_AIR);
	std::cout << "lol!!" << std::endl;
	for (auto entity : _world.entityManager.entities) {
		std::cout << (int)entity->type << std::endl;
		if (entity->type == EntityType::CREEPER) {
			auto finder = std::static_pointer_cast<MobileEntity>(entity);
			std::cout << "Setting goal to" << topPos << std::endl;
			finder->SetGoal(topPos);
		}
	}
}

}; // namespace Items