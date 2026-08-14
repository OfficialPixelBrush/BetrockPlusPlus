/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once

#include "AABB.h"
#include "block_shapes.h"
#include "inventory/item_stack.h"
#include "java/java_random.h"
#include "numeric_structs.h"
#include "packet_data.h"

class WorldManager;
struct Entity;
struct PlayerSession;

namespace Blocks {

bool GenericPlace(WorldManager& _world, Int3 _pos, [[maybe_unused]] Entity& _placer, PacketData::FaceDirection _face,
                  BlockType _blockId, uint8_t _meta);
void GenericBreak(WorldManager& _world, Int3 _pos, Entity& _destroyer);

struct BlockBehavior {
	// Called when we need to get the AABB for the selection box
	AABB (*getSelectionBox)(uint8_t _metadata) = DefaultAabb;

	// Called when we need to check for ray intersections for selection
	AABB (*getRayBounds)(uint8_t _metadata) = DefaultAabb;

	// Called when we need to check the collision of this block
	CollisionShape (*getCollider)(uint8_t _metadata) = DefaultCollider;

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
	// Returns if the placement was successful
	bool (*onBlockPlaced)(WorldManager& _world, Int3 _pos, Entity& _placer, PacketData::FaceDirection _face,
	                      BlockType _blockId, uint8_t _meta) = GenericPlace;

	// Called when player breaks the block
	void (*onBlockDestroyedByPlayer)(WorldManager& _world, Int3 _pos, Entity& _destroyer) = GenericBreak;

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

void RegisterBlockBehaviors();

std::vector<ItemStack> GetBlockDrops(BlockType _blockId, uint8_t _meta, Java::Random& _rng);

extern BlockBehavior blockBehaviors[BLOCK_MAX];
}; // namespace Blocks