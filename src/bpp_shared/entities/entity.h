/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "base_types.h"
#include "blocks/block_properties.h"
#include "dimensions.h"
#include "entities.h"
#include "helpers/AABB.h"
#include "nbt/nbt.h"
#include "numeric_structs.h"
#include "packet_data.h"
#include <vector>

// Constants pulled from the betaWiki!
// https://pixelbrush.dev/beta-wiki/entities/movement
// I <3 BETA WIKI!
const float KNOCKBACK_VELOCITY_DAMPENING = 0.5f;
const float HORIZONTAL_KNOCKBACK = 0.4f;
const float VERTICAL_KNOCKBACK = 0.4f;

const float COBWEB_VERTICAL_DRAG = 0.05f;
const float COBWEB_HORIZONTAL_DRAG = 0.25f;

const float LADDER_MAX_HORIZONTAL = 0.15f;
const float LADDER_MAX_DESCENT = 0.15f;
const float LADDER_SNEAK_DESCENT = 0.0f;
const float LADDER_WALL_BOOST = 0.2f;

const float WATER_DRAG = 0.8f;
const float LAVA_DRAG = 0.5f;
const float FLUID_GRAVITY = 0.02f;
const float FLUID_ACCELERATION = 0.02f;
const float FLUID_WALL_BOOST = 0.3f;

const float GRAVITY = 0.08f;
const float VERTICAL_FRICTION = 0.98f;
const float HORIZONTAL_FRICTION = 0.91f;
const float JUMP_VELOCITY = 0.42f;
const float FLUID_JUMP_BOOST = 0.04f;
const float FALL_DAMAGE_FLOOR = 3.0f;
const float STEP_HEIGHT = 0.5f;
const float DEFAULT_BLOCK_SLIPPERINESS = 0.6f;
const float NORMAL_FRICTION_CUBED = 0.16277136f;
const float AIR_ACCELERATION = 0.02f;
const float INPUT_DECAY = 0.98f;
const float SNEAK_SPEED_MODIFIER = 0.3f;

struct PlayerEntity;
struct EntityManager;
struct Entity {
	// For randomness
	Java::Random m_rand;

	// Entity type because notch split stuff into multiple packets based on type
	EntityType m_type = EntityType::NONE;

	// World pointer
	WorldManager* m_world = nullptr;
	EntityManager* m_entityManager = nullptr;

	// Identity
	EntityId m_id = -1; // -1 = not yet spawned
	bool m_isDead = false;
	TickTime m_ticksExisted = 0;
	Dimension m_dim = Dimension::Overworld;

	// Riding
	Entity* m_vehicle = nullptr;
	Entity* m_passenger = nullptr;

	Vec3 m_position;
	Vec3 m_velocity;
	bool m_forceVelocityUpdate = false;

	// Look direction
	float m_rotationYaw = 0.0f;
	float m_rotationPitch = 0.0f;

	// Collision
	AABB m_collider;
	Int3 m_bucketPos; // The bucket this entity is currently in (for spatial partitioning)
	Blocks::BlockProperties m_belowBlock;

	// Width/height of the collision box in blocks.
	float m_width = 0.6f;
	float m_height = 1.8f;

	// Vertical offset from position.y down to the bottom of the bounding box
	float m_yOffset = 0.0f;

	// How high a block face this entity can step onto without jumping.
	float m_stepHeight = 0.0f;

	// Collision state
	bool m_onGround = false;
	bool m_collided = false;
	bool m_collidedHorizontally = false;
	bool m_collidedVertically = false;

	// Movement / environment state
	bool m_hasPhysics = true;
	bool m_inWeb = false; // Inside a cobweb
	bool m_inWater = false;
	bool m_inLava = false;
	bool m_onLadder = false;

	float m_fallDistance = 0.0f;
	int m_nextStepDistance = 0;

	// Accumulated walk distance this tick (unused rn its mostly for the client)
	float m_distanceWalkedModified = 0.0f;
	float m_ySize = 0.0f;

	// Inputs
	Float2 m_input; // Think of this like a 2D joystick (up/down Y -> forward/backward, right/left X -> strafe)
	bool m_sneaking = false;
	bool m_jumping = false;

	// Fire
	int m_fireTicks = 0;           // Ticks remaining on fire; 0 = not on fire
	bool m_inFire = false;         // Currently touching a fire/lava block
	int m_fireResistance = 1;      // Ticks of immunity after catching fire
	bool m_isImmuneToFire = false; // Total fire immunity

	// Combat
	bool m_beenAttacked = false;
	int m_hurtResistantTime = 0;  // Invincibility frames after being hit
	float m_attackedAtYaw = 0.0f; // Yaw from which the last attack came

	// Spawning
	bool m_preventEntitySpawning = false;
	bool m_isFirstUpdate = true; // True only on the very first tick

	// Air
	int m_maxAir = 300;
	int m_air = 300;

	Entity() {
		rebuildCollider();
	}
	virtual ~Entity() = default;

	// Encode Entity info into relevant Metadata
	virtual void encodeMetadata([[maybe_unused]] const std::vector<PacketData::EntityMetadata::DataEntry>& metadata) {}

	// Apply Metadata to Entity
	virtual void decodeMetadata([[maybe_unused]] const std::vector<PacketData::EntityMetadata::DataEntry>& metadata) {}

	virtual void tick();

	virtual bool canBePushed() {
		return false;
	}

	void rebuildCollider() {
		double halfWidth = double(m_width) / 2.0;
		double bottom = m_position.m_y - double(m_yOffset) + double(m_ySize);
		m_collider = { m_position.m_x - halfWidth,  bottom,
			         m_position.m_z - halfWidth,  m_position.m_x + halfWidth,
			         bottom + double(m_height), m_position.m_z + halfWidth };
	}

	void teleport(Vec3 newpos, Vec2 newrot = { 0, 0 }) {
		m_position.m_x = newpos.m_x;
		m_position.m_y = newpos.m_y;
		m_position.m_z = newpos.m_z;
		m_rotationYaw = newrot.m_x;
		m_rotationPitch = newrot.m_y;
		m_ySize = 0.0f;
		rebuildCollider();
	}

	virtual bool attackEntityFrom(Entity* entity, int damage) {
		m_beenAttacked = true;
		m_forceVelocityUpdate = true;
		return false;
	}
	virtual AABB getFluidCollider() {
		// Returns the collider we use to compare if we are in a fluid
		return m_collider.expand(0.0, -0.4, 0.0);
	}
	virtual AABB getLavaCollider() {
		// Returns the collider we use to detect if we are in lava
		return m_collider.expand(-0.1, -0.4, -0.1);
	}
	virtual bool pushOutOfBlocks(Vec3 pos);
	virtual void onCollideWithPlayer(PlayerEntity& entity);
	virtual void applyKnockback(Vec3 direction);
	virtual void applyInput(float acceleration);
	virtual void move(Vec3& velocity);
	//TODO: Move to LivingEntity
	virtual void dealDamage(int amount);
	virtual void updateFallState(float movedY);
	virtual std::optional<Tag> serializeToNBT();
	virtual void loadFromNBT(Tag& nbt);
};