/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "tool_properties.h"
#include "blocks.h"
#include "blocks/block_properties.h"
#include "entities/entity.h"
#include "entities/entity_mobile.h"
#include "items.h"
#include "logger.h"

namespace Items {
std::unordered_map<ItemId, ToolProperties> toolProperties = {};
std::unordered_map<ItemId, ToolBehavior> toolBehavior = {};

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

static bool InflictDamage(Entity& _targetEntity, Entity& _sourceEntity, EntityHealth _damage) {
	return _targetEntity.AttackEntityFrom(&_sourceEntity, _damage);
}

bool IsEffective(ToolType _type, BlockType _block) {
	switch (_type) {
	case ToolType::Pickaxe:
		return (_block == BLOCK_COBBLESTONE || _block == BLOCK_DOUBLE_SLAB || _block == BLOCK_SLAB ||
		        _block == BLOCK_STONE || _block == BLOCK_SANDSTONE || _block == BLOCK_COBBLESTONE_MOSSY ||
		        _block == BLOCK_ORE_IRON || _block == BLOCK_IRON || _block == BLOCK_ORE_COAL ||
		        _block == BLOCK_ORE_GOLD || _block == BLOCK_GOLD || _block == BLOCK_ORE_DIAMOND ||
		        _block == BLOCK_DIAMOND || _block == BLOCK_ICE || _block == BLOCK_NETHERRACK ||
		        _block == BLOCK_ORE_LAPIS_LAZULI || _block == BLOCK_LAPIS_LAZULI);
	case ToolType::Axe:
		return (_block == BLOCK_PLANKS || _block == BLOCK_BOOKSHELF || _block == BLOCK_LOG || _block == BLOCK_CHEST);
	case ToolType::Shovel:
		return (_block == BLOCK_GRASS || _block == BLOCK_DIRT || _block == BLOCK_SAND || _block == BLOCK_GRAVEL ||
		        _block == BLOCK_SNOW_LAYER || _block == BLOCK_SNOW || _block == BLOCK_CLAY ||
		        _block == BLOCK_FARMLAND);
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

float ToolEffectiveness(ItemStack* _stack, BlockType _block) {
	if (!_stack || !toolProperties.contains(_stack->id))
		return 1.0f;
	const auto& props = toolProperties[_stack->id];
	if (IsEffective(props.type, _block))
		return MaterialToEfficiency(props.material);
	return 1.0f;
}

float SwordEffectiveness(ItemStack* /*_stack*/, BlockType _block) {
	// ItemSword.GetStrengthAgainstBlock: cobweb 15, everything else 1.5
	return (_block == BLOCK_COBWEB) ? 15.0f : 1.5f;
}

float ShearsEffectiveness(ItemStack* /*_stack*/, BlockType _block) {
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

bool CanPlayerHarvest(ItemStack* _stack, BlockType _targetBlock) {
	// InventoryPlayer.canHarvestBlock: material.isHarvestable OR held.canHarvestBlock
	if (Blocks::blockProperties[_targetBlock].material.isHarvestable)
		return true;
	if (!_stack || !toolProperties.contains(_stack->id))
		return false;
	const auto& props = toolProperties[_stack->id];
	if (!props.canHarvest)
		return false;
	return props.canHarvest(MaterialToLevel(props.material), _targetBlock);
}

float GetStrengthAgainstBlock(ItemStack* _stack, BlockType _targetBlock) {
	if (!_stack || !toolProperties.contains(_stack->id))
		return 1.0f;
	const auto& props = toolProperties[_stack->id];
	if (props.howEffectiveAgainstBlock)
		return props.howEffectiveAgainstBlock(_stack, _targetBlock);
	return 1.0f;
}

// If hardness < 0 -> 0
// If can't Harvest -> 1 / hardness / 100   (no water/air penalty)
// If can Harvest -> strengthVsBlock / hardness / 30
// Water and not onGround each divide strengthVsBlock by 5
float BlockDamagePerTick(ItemStack* _stack, BlockType _targetBlock, bool _inWater, bool _onGround) {
	float hardness = Blocks::blockProperties[_targetBlock].hardness;
	if (hardness < 0.0f)
		return 0.0f;
	if (hardness == 0.0f)
		return 1.0f;

	if (!CanPlayerHarvest(_stack, _targetBlock))
		return 1.0f / hardness / 100.0f;

	float efficiency = GetStrengthAgainstBlock(_stack, _targetBlock);
	if (_inWater) {
		efficiency /= 5.0f;
	}
	if (!_onGround) {
		efficiency /= 5.0f;
	}
	return efficiency / hardness / 30.0f;
}

void AttackWithItem(Entity& _targetEntity, Entity& _sourceEntity, ItemStack* _stack) {
	EntityHealth damage = 1;
	if (toolProperties.contains(_stack->id))
		damage = CalculateDamage(toolProperties[_stack->id].type, MaterialToLevel(toolProperties[_stack->id].material));
	bool canInflictDamage = InflictDamage(_targetEntity, _sourceEntity, damage);
	
	// Apparently vanilla doesn't do this for some reason
	/*
	if (!canInflictDamage) {
		return;
	}
	*/

	// ItemSword.hitEntity damages 1; ItemTool.hitEntity damages 2
	int durabilityLoss = 1;
	if (toolProperties.contains(_stack->id)) {
		switch (toolProperties[_stack->id].type) {
		case ToolType::Pickaxe:
		case ToolType::Axe:
		case ToolType::Shovel:
			durabilityLoss = 2;
			break;
		default:
			break;
		}
	}
	HarmTool(_stack, durabilityLoss);
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

void HarmTool(ItemStack* _stack, const int _amount) {
	if (!_stack || _amount <= 0 || !toolProperties.contains(_stack->id))
		return;
	_stack->data = static_cast<ItemDamage>(_stack->data + _amount);
	if (_stack->data >= toolProperties[_stack->id].maxUses) {
		_stack->DecrementCount(1);
	}
}

void OnToolFinishMining(ItemStack* _stack, BlockType _targetBlock) {
	if (!_stack || !toolProperties.contains(_stack->id))
		return;
	const auto& props = toolProperties[_stack->id];
	switch (props.type) {
	case ToolType::Sword:
		HarmTool(_stack, 2);
		return;
	case ToolType::Pickaxe:
	case ToolType::Axe:
	case ToolType::Shovel:
		HarmTool(_stack, 1);
		return;
	case ToolType::Hoe:
		return;
	case ToolType::None:
		if (_stack->id == Items::SHEARS &&
		    (_targetBlock == BLOCK_LEAVES || _targetBlock == BLOCK_COBWEB)) {
			HarmTool(_stack, 1);
		}
		return;
	}
}

void UseHoe(WorldManager& _world, ItemStack* _stack, Int3 _pos, Entity& _user, PacketData::FaceDirection _face) {
	BlockType b = _world.GetBlockId(_pos);
	if (b == BLOCK_GRASS || b == BLOCK_DIRT) {
		_world.SetBlock(_pos, BLOCK_FARMLAND);
	}
	HarmTool(_stack, 1);
}

void UseFlintAndSteel(WorldManager& _world, ItemStack* _stack, Int3 _pos, Entity& _user,
                      PacketData::FaceDirection _face) {
	if (_user.sneaking) {
		TestSetGoal(_world,_stack,_pos,_face);
		return;
	}
	_pos = Blocks::GetAdjacentBlockPos(_pos, _face);
	_world.SetBlock(_pos, BLOCK_FIRE);
	HarmTool(_stack, 1);
}

void TestSetGoal(WorldManager& _world, ItemStack* _stack, Int3 _pos, PacketData::FaceDirection _face) {
	Int3 topPos = _pos;
	topPos.y += 1;
	_world.SetBlock(topPos, BLOCK_AIR);
	std::cout << "lol!!" << std::endl;
	for (auto& entity : _world.entityManager.entities) {
		std::cout << (int)entity->type << std::endl;
		if (entity->type == EntityType::PIG) {
			auto finder = std::static_pointer_cast<MobileEntity>(entity);
			std::cout << "Setting goal to" << topPos << std::endl;
			finder->SetGoal(topPos);
		}
	}
}

}; // namespace Items