#include "items.h"
#include "inventory.h"

std::shared_ptr<NbtTag> NbtItem(int8_t slot, int16_t id, int8_t count, int16_t damage) {
	auto invSlot = std::make_shared<CompoundNbtTag>(std::to_string(static_cast<int32_t>(slot)));
	invSlot->Put(std::make_shared<ByteNbtTag>("Slot", slot));
	invSlot->Put(std::make_shared<ShortNbtTag>("id", id));
	invSlot->Put(std::make_shared<ByteNbtTag>("Count", count));
	invSlot->Put(std::make_shared<ShortNbtTag>("Damage", damage));
	return invSlot;
}

bool IsHoe(int16_t id) {
	return (id == ITEM_HOE_DIAMOND || id == ITEM_HOE_GOLD || id == ITEM_HOE_IRON || id == ITEM_HOE_STONE || id == ITEM_HOE_WOOD);
}

bool IsSword(int16_t id) {
	return (id == ITEM_SWORD_DIAMOND || id == ITEM_SWORD_GOLD || id == ITEM_SWORD_IRON || id == ITEM_SWORD_STONE || id == ITEM_SWORD_WOOD);
}

bool IsPickaxe(int16_t id) {
	return (id == ITEM_PICKAXE_DIAMOND || id == ITEM_PICKAXE_GOLD || id == ITEM_PICKAXE_IRON || id == ITEM_PICKAXE_STONE || id == ITEM_PICKAXE_WOOD);
}

bool IsAxe(int16_t id) {
	return (id == ITEM_AXE_DIAMOND || id == ITEM_AXE_GOLD || id == ITEM_AXE_IRON || id == ITEM_AXE_STONE || id == ITEM_AXE_WOOD);
}

bool IsShovel(int16_t id) {
	return (id == ITEM_SHOVEL_DIAMOND || id == ITEM_SHOVEL_GOLD || id == ITEM_SHOVEL_IRON || id == ITEM_SHOVEL_STONE || id == ITEM_SHOVEL_WOOD);
}

bool IsWeapon(int16_t id) {
	return (IsSword(id) || id == ITEM_BOW);
} 

bool IsTool(int16_t id) {
	return IsShovel(id) || IsAxe(id) || IsPickaxe(id) || IsHoe(id) || id == ITEM_FLINT_AND_STEEL;
}

bool CanDamage(int16_t id) {
	return ( IsTool(id) || IsWeapon(id) );
}

bool IsThrowable(int16_t id) {
	return ( id == ITEM_SNOWBALL || id == ITEM_EGG );
}

bool CanPlace(int16_t id) {
	return (id == ITEM_SEEDS_WHEAT || id == ITEM_BED || id == ITEM_BOAT || id == ITEM_BUCKET_WATER ||
		id == ITEM_BUCKET_LAVA || id == ITEM_CAKE || id == ITEM_DOOR_IRON || id == ITEM_DOOR_WOOD);
}

bool IsItem(int16_t id) {
	return (id > ITEM_MINIMUM && id < ITEM_MAX);
}

bool IsBlock(int16_t id) {
	return (id > BLOCK_INVALID && id < BLOCK_MAX);
}

int16_t GetDamage(int16_t id) {
	switch(id) {
		default:
			return 0;
	}
}

int32_t GetMaxStack(int16_t id) {
	if (IsBlock(id))
		return MAX_BLOCK_STACK;
	if (IsTool(id))
		return MAX_TOOL_STACK;
	return MAX_ITEM_STACK;
}