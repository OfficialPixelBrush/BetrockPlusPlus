/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "tile_entity.h"
#include "blocks.h"
#include "entities/entity_pig.h"
#include "entities/entity_skeleton.h"
#include "entities/entity_spider.h"
#include "entities/entity_zombie.h"
#include "inventory/item_stack.h"
#include "items.h"
#include "items/item_properties.h"
#include "world/world.h"
#include <string>

// TODO: Move all these into separate files just like InventoryInteraction
void TileEntity::Tick(WorldManager& /*_world*/) {
	// no-op
	return;
}

void TileEntityChest::Tick(WorldManager& /*_world*/) {
	if (chunk && inventory.isModified) {
		chunk->isModified = true;
		inventory.isModified = false;
	}
}

bool TileEntityMobSpawner::PlayerInRange(WorldManager& _world) {
	return _world.entityManager.GetClosestPlayerWithin(Vec3{ position.x + 0.5, position.y + 0.5, position.z + 0.5 },
	                                                   16) != nullptr;
}

void TileEntityMobSpawner::Tick(WorldManager& _world) {
	if (!PlayerInRange(_world))
		return;

	auto updateDelay = [](Java::Random& _rand) -> int {
		return 200 + _rand.NextInt(600);
	};

	// Why not <= 0? Because notch wrote this as "delay == -1"..
	// For some reason
	if (this->delay < 0)
		this->delay = updateDelay(_world.rand);
	else {
		this->delay--;
		return;
	}

	for (size_t attempt = 0; attempt < 4; attempt++) {
		// TODO: make a helper in the entity manager for this
		std::shared_ptr<Entity> myEntity;
		EntityType checkType = EntityType::NONE;
		if (this->entityId == "Skeleton") {
			myEntity = std::make_shared<SkeletonEntity>();
			checkType = EntityType::SKELETON;
		} else if (this->entityId == "Zombie") {
			myEntity = std::make_shared<ZombieEntity>();
			checkType = EntityType::ZOMBIE;
		} else if (this->entityId == "Spider") {
			myEntity = std::make_shared<SpiderEntity>();
			checkType = EntityType::SPIDER;
		} else {
			myEntity = std::make_shared<PigEntity>();
			checkType = EntityType::PIG;
		}

		if (!myEntity)
			return;

		AABB searchBox = {
			double(position.x),       double(position.y),       double(position.z),
			double(position.x) + 1.0, double(position.y) + 1.0, double(position.z) + 1.0,
		};
		auto numEntities =
		    _world.entityManager.GetEntitiesWithinAabbOfType(searchBox.Expand(8.0, 4.0, 8.0), checkType).size();
		if (numEntities >= 6) {
			this->delay = updateDelay(_world.rand);
			return;
		}

		Vec3 spawn = {};
		spawn.x = double(position.x) + (_world.rand.NextDouble() - _world.rand.NextDouble()) * 4.0;
		spawn.y = double(position.y) + _world.rand.NextInt(3) - 1;
		spawn.z = double(position.z) + (_world.rand.NextDouble() - _world.rand.NextDouble()) * 4.0;

		auto mobEntity = dynamic_cast<MobEntity*>(myEntity.get());

		mobEntity->world = &_world;
		mobEntity->entityManager = &_world.entityManager;
		mobEntity->Teleport(spawn, { _world.rand.NextFloat() * 360.0f, 0 });

		// Maybe not vanilla accurate?
		// This seems to improve spawn rates, though
		mobEntity->rand.SetSeed(_world.rand.NextLong());
		if (mobEntity->CanSpawnAt()) {
			_world.entityManager.AddEntity(std::move(myEntity));
			this->delay = updateDelay(_world.rand);
		}
	}
}

static ItemStack GetSmeltingResult(ItemId& _input) {
	switch (_input) {
	case BLOCK_ORE_IRON:
		return { Items::IRON, 1, 0 };
	case BLOCK_ORE_GOLD:
		return { Items::GOLD, 1, 0 };
	case BLOCK_ORE_DIAMOND:
		return { Items::DIAMOND, 1, 0 };
	case BLOCK_SAND:
		return { BLOCK_GLASS, 1, 0 };
	case Items::PORKCHOP:
		return { Items::PORKCHOP_COOKED, 1, 0 };
	case Items::FISH:
		return { Items::FISH_COOKED, 1, 0 };
	case BLOCK_COBBLESTONE:
		return { BLOCK_STONE, 1, 0 };
	case Items::CLAY:
		return { Items::BRICK, 1, 0 };
	case BLOCK_CACTUS:
		return { Items::DYE, 1, 2 }; // Green dye
	case BLOCK_LOG:
		return { Items::COAL, 1, 1 }; // Charcoal
	}

	return ItemStack{};
}

static bool CanAcceptSmeltResult(Inventory& _inventory, const ItemStack& _result) {
	ItemStack& output = _inventory.slots[2];

	if (output.id == Items::INVALID)
		return true;
	if (output.id != _result.id || output.data != _result.data)
		return false;

	int maxStack = Items::GetMaxStack(output.id);
	return (output.count + _result.count) <= maxStack;
}

void TileEntityFurnace::Tick(WorldManager& _world) {
	dirtyFlags = FLAG_NONE;

	bool hasInput = inventory.slots[0].id != Items::INVALID && inventory.slots[0].count > 0;
	ItemStack result = hasInput ? GetSmeltingResult(inventory.slots[0].id) : ItemStack{ Items::INVALID };
	bool canSmelt = hasInput && result.id != Items::INVALID && CanAcceptSmeltResult(inventory, result);

	bool wasBurning = burnTime > 0;

	{
		const int oldBurnTime = burnTime;

		if (burnTime > 0)
			--burnTime;

		if (burnTime == 0 && canSmelt) {
			const int maxBurnTime = GetMaxBurnTime();
			if (maxBurnTime != lastMaxBurnTime && maxBurnTime != 0) {
				dirtyFlags |= FLAG_MAX_BURN_TIME;
				lastMaxBurnTime = maxBurnTime;
			}

			burnTime = maxBurnTime;
			if (burnTime > 0)
				inventory.DecreaseStackSize(1, 1);
		}

		if (burnTime != oldBurnTime)
			dirtyFlags |= FLAG_BURN_TIME;
	}

	if (wasBurning != (burnTime > 0)) {
		// Flip furnace block ID
		auto oldMeta = _world.GetMetadata(this->position);
		_world.SetBlock(this->position, BlockType(61 + (burnTime > 0)), oldMeta, /*Keep Tile Entity=*/true);
	}

	{
		const int oldCookTime = cookTime;

		if (burnTime > 0 && canSmelt) {
			if (++cookTime >= 200) {
				inventory.MergeItemStackInInventory(result, false, 2, 2);
				inventory.DecreaseStackSize(0, 1);
				cookTime = 0;
			}
		} else {
			cookTime = 0;
		}

		if (cookTime != oldCookTime)
			dirtyFlags |= FLAG_COOK_TIME;
	}

	if (chunk && inventory.isModified) {
		chunk->isModified = true;
		inventory.isModified = false;
	}
}

int TileEntityFurnace::GetMaxBurnTime() const {
	const ItemId fuel = inventory.slots[1].id;

	switch (fuel) {
	case Items::COAL:
		return 1600;
	case Items::STICK:
	case BLOCK_SAPLING:
		return 100;
	case Items::BUCKET_LAVA:
		return 20000;
	}

	if (Items::IsBlock(fuel) && Blocks::blockProperties[fuel].material == Material::Wood()) {
		return 300;
	}

	return 0;
}

int TileEntityFurnace::GetBurnTime() const {
	return burnTime;
}

int TileEntityFurnace::GetCookTime() const {
	return cookTime;
}

void TileEntityDispenser::Tick(WorldManager& /*_world*/) {
	if (chunk && inventory.isModified) {
		chunk->isModified = true;
		inventory.isModified = false;
	}
}

constexpr std::string GetTileNbtId(TileType _type) {
	switch (_type) {
	case TileType::CHEST:
		return "Chest";
	case TileType::DISPENSER:
		return "Trap";
	case TileType::FURNACE:
		return "Furnace";
	case TileType::SIGN:
		return "Sign";
	case TileType::SPAWNER:
		return "MobSpawner";
	}
	// Invalid type, empty string
	return "";
}

Tag TileEntity::Serialize() {
	auto root = Tag{};
	root.type = TAG_COMPOUND;

	auto id = Tag{ .type = TAG_STRING, .name = "id", .longValue = 0, .stringValue = GetTileNbtId(type) };
	auto x = Tag{ .type = TAG_INT, .name = "x", .intValue = position.x };
	auto y = Tag{ .type = TAG_INT, .name = "y", .intValue = position.y };
	auto z = Tag{ .type = TAG_INT, .name = "z", .intValue = position.z };

	root.compound["id"] = id;
	root.compound["x"] = x;
	root.compound["y"] = y;
	root.compound["z"] = z;

	return root;
}

Tag TileEntityChest::Serialize() {
	auto root = TileEntity::Serialize();

	// Construct our inventory
	auto items = Tag{ .type = TAG_LIST, .name = "Items", .longValue = 0, .listType = TAG_COMPOUND };
	int8_t currentSlot = 0;
	for (auto& stack : inventory.slots) {
		if (stack.id != Items::Id::INVALID) {
			auto item = Tag{ .type = TAG_COMPOUND, .name = "", .longValue = 0 };
			auto count = Tag{ .type = TAG_BYTE, .name = "Count", .byteValue = stack.count };
			auto damage = Tag{ .type = TAG_SHORT, .name = "Damage", .shortValue = stack.data };
			auto id = Tag{ .type = TAG_SHORT, .name = "id", .shortValue = stack.id };
			auto slot = Tag{ .type = TAG_BYTE, .name = "Slot", .byteValue = currentSlot };

			item.compound["Count"] = count;
			item.compound["Damage"] = damage;
			item.compound["id"] = id;
			item.compound["Slot"] = slot;

			items.list.push_back(item);
		}
		currentSlot++;
	}

	root.compound["Items"] = items;

	return root;
}

Tag TileEntityFurnace::Serialize() {
	auto root = TileEntity::Serialize();

	// Construct our inventory
	auto items = Tag{ .type = TAG_LIST, .name = "Items", .longValue = 0, .listType = TAG_COMPOUND };
	int8_t currentSlot = 0;
	for (auto& stack : inventory.slots) {
		if (stack.id != Items::Id::INVALID) {
			auto item = Tag{ .type = TAG_COMPOUND, .name = "", .longValue = 0 };
			auto count = Tag{ .type = TAG_BYTE, .name = "Count", .byteValue = stack.count };
			auto damage = Tag{ .type = TAG_SHORT, .name = "Damage", .shortValue = stack.data };
			auto id = Tag{ .type = TAG_SHORT, .name = "id", .shortValue = stack.id };
			auto slot = Tag{ .type = TAG_BYTE, .name = "Slot", .byteValue = currentSlot };

			item.compound["Count"] = count;
			item.compound["Damage"] = damage;
			item.compound["id"] = id;
			item.compound["Slot"] = slot;

			items.list.push_back(item);
		}
		currentSlot++;
	}

	root.compound["Items"] = items;

	return root;
}

Tag TileEntityDispenser::Serialize() {
	auto root = TileEntity::Serialize();

	// Construct our inventory
	auto items = Tag{ .type = TAG_LIST, .name = "Items", .longValue = 0, .listType = TAG_COMPOUND };
	int8_t currentSlot = 0;
	for (auto& stack : inventory.slots) {
		if (stack.id != Items::Id::INVALID) {
			auto item = Tag{ .type = TAG_COMPOUND, .name = "", .longValue = 0 };
			auto count = Tag{ .type = TAG_BYTE, .name = "Count", .byteValue = stack.count };
			auto damage = Tag{ .type = TAG_SHORT, .name = "Damage", .shortValue = stack.data };
			auto id = Tag{ .type = TAG_SHORT, .name = "id", .shortValue = stack.id };
			auto slot = Tag{ .type = TAG_BYTE, .name = "Slot", .byteValue = currentSlot };

			item.compound["Count"] = count;
			item.compound["Damage"] = damage;
			item.compound["id"] = id;
			item.compound["Slot"] = slot;

			items.list.push_back(item);
		}
		currentSlot++;
	}

	root.compound["Items"] = items;

	return root;
}

Tag TileEntitySign::Serialize() {
	auto root = TileEntity::Serialize();

	auto vText1 = Tag{ .type = TAG_STRING, .name = "Text1", .longValue = 0, .stringValue = text1 };
	auto vText2 = Tag{ .type = TAG_STRING, .name = "Text2", .longValue = 0, .stringValue = text2 };
	auto vText3 = Tag{ .type = TAG_STRING, .name = "Text3", .longValue = 0, .stringValue = text3 };
	auto vText4 = Tag{ .type = TAG_STRING, .name = "Text4", .longValue = 0, .stringValue = text4 };

	root.compound["Text1"] = vText1;
	root.compound["Text2"] = vText2;
	root.compound["Text3"] = vText3;
	root.compound["Text4"] = vText4;

	return root;
}

Tag TileEntityMobSpawner::Serialize() {
	auto root = TileEntity::Serialize();

	auto vEntityId = Tag{ .type = TAG_STRING, .name = "EntityId", .longValue = 0, .stringValue = entityId };
	auto vDelay = Tag{ .type = TAG_SHORT, .name = "Delay", .shortValue = delay };

	root.compound["EntityId"] = vEntityId;
	root.compound["Delay"] = vDelay;

	return root;
}