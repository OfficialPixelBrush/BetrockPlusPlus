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
#include "networking/packets.h"
#include <numeric_structs.h>
#include <vector>

struct WorldManager;
struct Entity;
struct PlayerSession;

namespace Blocks {

Int3 GetAdjacentBlockPos(Int3 _pos, PacketData::FaceDirection _face);
bool CanSugarcaneSurviveAt(WorldManager& _world, Int3 _pos);
float GetFluidPercentAir(uint8_t _meta);
void BreakAndDropBlock(WorldManager& _world, Int3 _pos);

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
	AABB (*getSelectionBox)(uint8_t _metadata) = nullptr;

	// Called when we need to check for ray intersections for selection
	AABB (*getRayBounds)(uint8_t _metadata) = nullptr;

	// Called when we need to check the collision of this block
	CollisionShape (*getCollider)(uint8_t _metadata) = nullptr;

	// Called each random Tick if ticksOnLoad = true
	// Also called for scheduled ticks
	void (*onTick)(WorldManager& _world, Int3 _pos, uint8_t _meta, Java::Random& _random) = nullptr;

	// Called when block is placed by world gen or setBlock
	void (*onBlockAdded)(WorldManager& _world, Int3 _pos) = nullptr;

	// Called when block is removed
	void (*onBlockRemoval)(WorldManager& _world, Int3 _pos) = nullptr;

	// Called when a neighboring block changes
	void (*onNeighborBlockChange)(WorldManager& _world, Int3 _pos) = nullptr;

	// Called when a player left-clicks the block (not breaks, just clicks)
	// pos is where that block that is interacted with is
	void (*onBlockClicked)(WorldManager& _world, Int3 _pos) = nullptr;

	// Called when a player right-clicks the block
	// Return true if we allow the player to still place their held block
	bool (*onBlockActivated)(WorldManager& _world, Int3 _pos) = nullptr;

	// Called when block is placed by a player
	void (*onBlockPlaced)(WorldManager& _world, Int3 _pos, Entity& _placer, PacketData::FaceDirection _face) = nullptr;

	// Called when player breaks the block
	void (*onBlockDestroyedByPlayer)(WorldManager& _world, Int3 _pos, Entity& _destroyer) = nullptr;

	// Called when an explosion destroys the block
	void (*onBlockDestroyedByExplosion)(WorldManager& _world, Int3 _pos) = nullptr;

	// Called when an entity walks on top of the block
	void (*onEntityWalking)(WorldManager& _world, Int3 _pos, Entity& _entity) = nullptr;

	// Called when an entity collides with the block (cactus damage, etc.)
	void (*onEntityCollidedWithBlock)(WorldManager& _world, Int3 _pos, Entity& _entity) = nullptr;

	// Called when we need to find how this block would contribute to the push vector of an entity
	void (*velocityToAddToEntity)(WorldManager& _world, Int3 _pos, Vec3& _pushVector) = nullptr;

	// What item/block this drops when broken
	ItemId (*idDropped)(uint8_t _meta, Java::Random& _random) = nullptr;

	// The data value of the dropped item
	ItemDamage (*damageDropped)(uint8_t _meta) = nullptr;

	// How many items drop
	ItemAmount (*quantityDropped)(Java::Random& _random) = nullptr;
};

// Indexed by block ID, populated by registerAll()
extern BlockProperties blockProperties[256];
extern BlockBehavior blockBehaviors[256];

// Call once at startup before anything reads from the tables
void RegisterAll();

std::vector<ItemStack> GetBlockDrops(BlockType _blockId, uint8_t _meta, Java::Random& _rng);
} // namespace Blocks