#include "drops.h"

// Returns true if the destroyed item maintains its NBT data upon being dropped
bool KeepDamageOnDrop(int8_t id) { return id == BLOCK_WOOL || id == BLOCK_SLAB || id == BLOCK_DOUBLE_SLAB; }

// Returns true for all blocks that do not drop anything when they're destroyed
bool NoDrop(Item item) {
	return item.id == BLOCK_AIR || item.id == BLOCK_BEDROCK || item.id == BLOCK_WATER_FLOWING ||
		   item.id == BLOCK_WATER_STILL || item.id == BLOCK_LAVA_FLOWING || item.id == BLOCK_LAVA_STILL ||
		   item.id == BLOCK_GLASS || item.id == BLOCK_COBWEB || item.id == BLOCK_DEADBUSH ||
		   item.id == BLOCK_TALLGRASS || item.id == BLOCK_FIRE || item.id == BLOCK_MOB_SPAWNER ||
		   item.id == BLOCK_ICE || item.id == BLOCK_NETHER_PORTAL || item.id == BLOCK_CAKE;
}

// Returns the items that're dropped when a block is destroyed
Item GetDrop(Item item) {
	if (NoDrop(item))
		return Item{};
	int16_t damage = item.damage;
	if (!KeepDamageOnDrop(item.id))
		item.damage = 0;
	// By default, give back one of the same block
	switch (item.id) {
	case BLOCK_CROP_WHEAT:
		if (damage < MAX_CROP_SIZE) {
			item.id = ITEM_SEEDS_WHEAT;
		} else {
			item.id = ITEM_WHEAT;
		}
		break;
	case BLOCK_STONE:
		item.id = BLOCK_COBBLESTONE;
		break;
	case BLOCK_GRASS:
		item.id = BLOCK_DIRT;
		break;
	case BLOCK_SUGARCANE:
		item.id = ITEM_SUGARCANE;
		break;
	case BLOCK_ORE_COAL:
		item.id = ITEM_COAL;
		break;
	case BLOCK_LEAVES:
		item.id = BLOCK_SAPLING;
		item.amount = 1;
		break;
	case BLOCK_ORE_LAPIS_LAZULI:
		item.id = ITEM_DYE;
		// 4-8
		item.amount = 4;
		item.damage = 4;
		break;
	case BLOCK_BED:
		item.id = ITEM_BED;
		break;
	case BLOCK_REDSTONE:
		item.id = ITEM_REDSTONE;
		break;
	case BLOCK_ORE_DIAMOND:
		item.id = ITEM_DIAMOND;
		break;
	case BLOCK_SIGN:
	case BLOCK_SIGN_WALL:
		item.id = ITEM_SIGN;
		break;
	case BLOCK_DOOR_WOOD:
		item.id = ITEM_DOOR_WOOD;
		break;
	case BLOCK_DOOR_IRON:
		item.id = ITEM_DOOR_IRON;
		break;
	case BLOCK_ORE_REDSTONE_OFF:
	case BLOCK_ORE_REDSTONE_ON:
		item.id = ITEM_REDSTONE;
		// 4-5
		item.amount = 4;
		break;
	case BLOCK_REDSTONE_TORCH_OFF:
	case BLOCK_REDSTONE_TORCH_ON:
		item.id = BLOCK_REDSTONE_TORCH_ON;
		break;
	case BLOCK_CLAY:
		item.id = ITEM_CLAY;
		// ???
		item.amount = 4;
		break;
	case BLOCK_GLOWSTONE:
		item.id = ITEM_GLOWSTONE_DUST;
		// 2-4
		item.amount = 2;
		break;
	case BLOCK_REDSTONE_REPEATER_ON:
	case BLOCK_REDSTONE_REPEATER_OFF:
		item.id = BLOCK_REDSTONE_REPEATER_OFF;
		break;
	case BLOCK_DOUBLE_SLAB:
		item.id = BLOCK_SLAB;
		item.amount = 2;
		break;
	}
	return item;
}