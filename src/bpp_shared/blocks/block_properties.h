/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "enums/blocks.h"
#include "helpers/AABB.h"
#include "helpers/java/java_random.h"
#include "inventory/item_stack.h"
#include "materials.h"
#include <numeric_structs.h>
#include <vector>

struct WorldManager;
struct Entity;
struct PlayerSession;

namespace Blocks {
enum class StepSound : uint8_t {
	Stone, // default, also metal (different pitch)
	Wood,
	Gravel,
	Grass,
	Sand,
	Cloth,
	Glass,
};

struct BlockProperties {
	Material material = Material::Rock();
	StepSound stepSound = StepSound::Stone;

	uint8_t lightEmission = 0;  // 0-15
	uint8_t lightOpacity = 255; // 0 = transparent, 255 = fully opaque
	int tickRate = 10;

	float hardness = 1.0f;        // -1 = unbreakable (bedrock)
	float resistance = 5.0f;      // blast resistance
	float slipperiness = 0.6f;    // default friction, ice = 0.98f
	float particleGravity = 1.0f; // how fast break particles fall

	bool isCollidable = true;
	bool isOpaqueCube = true;
	bool isNormalCube = true;
	bool renderAsNormalBlock = true;
	bool ticksOnLoad = false;
	bool canBlockGrass = true;
	bool notifyNeighborsOnMetaChange = true;
	bool notifySelfOnMetaChange = true;
	bool enableStats = true; // false = breaking doesn't count for achievements
};

struct BlockBehavior {
	// Called when we need to get the AABB for the selection box
	AABB (*getSelectionBox)(uint8_t metadata) = nullptr;

	// Called when we need to check for ray intersections for selection
	AABB (*getRayBounds)(uint8_t metadata) = nullptr;

	// Called when we need to check the collision of this block
	CollisionShape (*getCollider)(uint8_t metadata) = nullptr;

	// Called each random tick if ticksOnLoad = true
	void (*onTick)(WorldManager&, Int3 pos, uint8_t meta, Java::Random& random) = nullptr;

	// Called when block is placed by world gen or setBlock
	void (*onBlockAdded)(WorldManager& world, Int3 pos) = nullptr;

	// Called when block is removed
	void (*onBlockRemoval)(WorldManager& world, Int3 pos) = nullptr;

	// Called when a neighboring block changes
	void (*onNeighborBlockChange)(WorldManager& world, Int3 pos) = nullptr;

	// Called when a player left-clicks the block (not breaks, just clicks)
	// pos is where that block that is interacted with is
	void (*onBlockClicked)(WorldManager& world, Int3 pos) = nullptr;

	// Called when a player right-clicks the block
	// Return true if we allow the player to still place their held block
	bool (*onBlockActivated)(WorldManager& world, Int3 pos) = nullptr;

	// Called when block is placed by a player
	void (*onBlockPlaced)(WorldManager& world, Int3 pos, Entity& placer) = nullptr;

	// Called when player breaks the block
	void (*onBlockDestroyedByPlayer)(WorldManager& world, Int3 pos, Entity& destroyer) = nullptr;

	// Called when an explosion destroys the block
	void (*onBlockDestroyedByExplosion)(WorldManager& world, Int3 pos) = nullptr;

	// Called when an entity walks on top of the block
	void (*onEntityWalking)(WorldManager& world, Int3 pos, Entity& entity) = nullptr;

	// Called when an entity collides with the block (cactus damage, etc.)
	void (*onEntityCollidedWithBlock)(WorldManager& world, Int3 pos, Entity& entity) = nullptr;

	// What item/block this drops when broken
	ItemId (*idDropped)(uint8_t meta, Java::Random& random) = nullptr;

	// The data value of the dropped item
	ItemDamage (*damageDropped)(uint8_t meta) = nullptr;

	// How many items drop
	ItemAmount (*quantityDropped)(Java::Random& random) = nullptr;
};

// Indexed by block ID, populated by registerAll()
extern BlockProperties blockProperties[256];
extern BlockBehavior blockBehaviors[256];

// Call once at startup before anything reads from the tables
void registerAll();

std::vector<ItemStack> getBlockDrops(BlockType blockId, uint8_t meta, Java::Random& rng);
} // namespace Blocks