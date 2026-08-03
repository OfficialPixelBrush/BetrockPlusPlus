/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "item_properties.h"
#include "base_types.h"
#include "enums/items.h"
#include "item_map.h"
#include "tool_properties.h"
#include <cstdint>

namespace Items {

// Global table definitions; declared extern in the header
std::unordered_map<ItemId, ItemBehavior> itemBehavior = {};
std::unordered_map<ItemId, ItemProperties> itemProperties = {};

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

ItemDamage GetMaxDurability(ItemId _id) {
	auto toolIt = toolProperties.find(_id);
	if (toolIt != toolProperties.end() && toolIt->second.maxUses > 0)
		return toolIt->second.maxUses;
	return 0;
}
}; // namespace Items