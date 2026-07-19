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

Int3 getAdjacentBlockPos(Int3 pos, PacketData::FaceDirection face);
bool canSugarcaneSurviveAt(WorldManager& world, Int3 pos);
float getFluidPercentAir(uint8_t meta);
void BreakAndDropBlock(WorldManager& world, Int3 pos);

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
	Material m_material = Material::Rock();
	StepSound m_stepSound = StepSound::Stone;

	uint8_t m_lightEmission = 0;  // 0-15
	uint8_t m_lightOpacity = 255; // 0 = transparent, 255 = fully opaque
	int m_tickRate = 10;

	float m_hardness = 1.0f;        // -1 = unbreakable (bedrock)
	float m_resistance = 5.0f;      // blast resistance
	float m_slipperiness = 0.6f;    // default friction, ice = 0.98f
	float m_particleGravity = 1.0f; // how fast break particles fall

	bool m_isCollidable = true;
	bool m_isOpaqueCube = true;
	bool m_isNormalCube = true;
	bool m_renderAsNormalBlock = true;
	bool m_ticksOnLoad = false;
	bool m_canBlockGrass = true;
	bool m_notifyNeighborsOnMetaChange = true;
	bool m_notifySelfOnMetaChange = true;
	bool m_enableStats = true; // false = breaking doesn't count for achievements
};

struct BlockBehavior {
	// Called when we need to get the AABB for the selection box
	AABB (*m_getSelectionBox)(uint8_t metadata) = nullptr;

	// Called when we need to check for ray intersections for selection
	AABB (*m_getRayBounds)(uint8_t metadata) = nullptr;

	// Called when we need to check the collision of this block
	CollisionShape (*m_getCollider)(uint8_t metadata) = nullptr;

	// Called each random tick if ticksOnLoad = true
	// Also called for scheduled ticks
	void (*m_onTick)(WorldManager& world, Int3 pos, uint8_t meta, Java::Random& random) = nullptr;

	// Called when block is placed by world gen or setBlock
	void (*m_onBlockAdded)(WorldManager& world, Int3 pos) = nullptr;

	// Called when block is removed
	void (*m_onBlockRemoval)(WorldManager& world, Int3 pos) = nullptr;

	// Called when a neighboring block changes
	void (*m_onNeighborBlockChange)(WorldManager& world, Int3 pos) = nullptr;

	// Called when a player left-clicks the block (not breaks, just clicks)
	// pos is where that block that is interacted with is
	void (*m_onBlockClicked)(WorldManager& world, Int3 pos) = nullptr;

	// Called when a player right-clicks the block
	// Return true if we allow the player to still place their held block
	bool (*m_onBlockActivated)(WorldManager& world, Int3 pos) = nullptr;

	// Called when block is placed by a player
	void (*m_onBlockPlaced)(WorldManager& world, Int3 pos, Entity& placer, PacketData::FaceDirection face) = nullptr;

	// Called when player breaks the block
	void (*m_onBlockDestroyedByPlayer)(WorldManager& world, Int3 pos, Entity& destroyer) = nullptr;

	// Called when an explosion destroys the block
	void (*m_onBlockDestroyedByExplosion)(WorldManager& world, Int3 pos) = nullptr;

	// Called when an entity walks on top of the block
	void (*m_onEntityWalking)(WorldManager& world, Int3 pos, Entity& entity) = nullptr;

	// Called when an entity collides with the block (cactus damage, etc.)
	void (*m_onEntityCollidedWithBlock)(WorldManager& world, Int3 pos, Entity& entity) = nullptr;

	// Called when we need to find how this block would contribute to the push vector of an entity
	void (*m_velocityToAddToEntity)(WorldManager& world, Int3 pos, Vec3& pushVector) = nullptr;

	// What item/block this drops when broken
	ItemId (*m_idDropped)(uint8_t meta, Java::Random& random) = nullptr;

	// The data value of the dropped item
	ItemDamage (*m_damageDropped)(uint8_t meta) = nullptr;

	// How many items drop
	ItemAmount (*m_quantityDropped)(Java::Random& random) = nullptr;
};

// Indexed by block ID, populated by registerAll()
extern BlockProperties blockProperties[256];
extern BlockBehavior blockBehaviors[256];

// Call once at startup before anything reads from the tables
void registerAll();

std::vector<ItemStack> getBlockDrops(BlockType blockId, uint8_t meta, Java::Random& rng);
} // namespace Blocks