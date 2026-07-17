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

// Forward declare
class WorldManager;

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
	Java::Random rand;

	// Entity type because notch split stuff into multiple packets based on type
	EntityType type = EntityType::NONE;

	// World pointer
	WorldManager* world = nullptr;
	EntityManager* entityManager = nullptr;

	// Identity
	EntityId id = -1; // -1 = not yet spawned
	bool isDead = false;
	TickTime ticksExisted = 0;
	Dimension dim = Dimension::Overworld;

	// Riding
	Entity* rider = nullptr;
	Entity* passenger = nullptr;

	// Position
	//TODO: use a Vec3 instead
	Vec3 position;

	// Velocity
	//TODO: use a Vec3 instead
	double motionX = 0.0;
	double motionY = 0.0;
	double motionZ = 0.0;
	bool forceVelocityUpdate = false;

	// Look direction
	float rotationYaw = 0.0f;
	float rotationPitch = 0.0f;

	// Collision
	AABB collider;
	Int3 bucketPos = { 0, 0, 0 }; // The bucket this entity is currently in (for spatial partitioning)
	Blocks::BlockProperties belowBlock;

	// Width/height of the collision box in blocks.
	float width = 0.6f;
	float height = 1.8f;

	// Vertical offset from position.y down to the bottom of the bounding box
	float yOffset = 0.0f;

	// How high a block face this entity can step onto without jumping.
	float stepHeight = 0.5f;

	// Collision state
	bool onGround = false;
	bool collided = false;
	bool collidedHorizontally = false;
	bool collidedVertically = false;
	bool sneaking = false;
	bool jumping = false;

	// Movement / environment state
	bool hasPhysics = true;
	bool inWeb = false; // Inside a cobweb
	bool inWater = false;
	bool inLava = false;
	bool onLadder = false;

	float fallDistance = 0.0f;

	int nextStepDistance = 0;

	// Accumulated walk distance this tick (unused rn its mostly for the client)
	float distanceWalkedModified = 0.0f;
	float ySize = 0.0f;

	//TODO: combine moveForward, moveStrafe and maybe jumping into an input vector
	float moveForward = 0.0f; // Forward/backward input axis
	float moveStrafe = 0.0f;  // Left/right input axis

	// Fire
	int fire = 0;                // Ticks remaining on fire; 0 = not on fire
	bool inFire = false;         // Currently touching a fire/lava block
	int fireResistance = 1;      // Ticks of immunity after catching fire
	bool isImmuneToFire = false; // Total fire immunity

	// Combat
	bool beenAttacked = false;
	int hurtResistantTime = 0;  // Invincibility frames after being hit
	float attackedAtYaw = 0.0f; // Yaw from which the last attack came

	// Spawning
	bool preventEntitySpawning = false;
	bool isFirstUpdate = true; // True only on the very first tick

	// Air
	int maxAir = 300;
	int air = 300;

	Entity() {
		rebuildCollider();
	}
	virtual ~Entity() = default;

	// Encode Entity info into relevant Metadata
	virtual void encodeMetadata([[maybe_unused]] const std::vector<PacketData::EntityMetadata::DataEntry>& metadata) {}

	// Apply Metadata to Entity
	virtual void decodeMetadata([[maybe_unused]] const std::vector<PacketData::EntityMetadata::DataEntry>& metadata) {}

	virtual void tick();

	void rebuildCollider() {
		double halfWidth = double(width) / 2.0;
		double bottom = position.y - double(yOffset) + double(ySize);
		collider = { position.x - halfWidth,  bottom,
			         position.z - halfWidth,  position.x + halfWidth,
			         bottom + double(height), position.z + halfWidth };
	}

	void teleport(Vec3 newpos, Vec2 newrot = { 0, 0 }) {
		position.x = newpos.x;
		position.y = newpos.y;
		position.z = newpos.z;
		rotationYaw = newrot.x;
		rotationPitch = newrot.y;
		ySize = 0.0f;
		rebuildCollider();
	}

	virtual bool attackEntityFrom(Entity* entity, int damage) {
		beenAttacked = true;
		forceVelocityUpdate = true;
		return false;
	}
	virtual AABB getFluidCollider() {
		// Returns the collider we use to compare if we are in a fluid
		return collider.expand(0.0, -0.4, 0.0);
	}
	virtual AABB getLavaCollider() {
		// Returns the collider we use to detect if we are in lava
		return collider.expand(-0.1, -0.4, -0.1);
	}
	virtual bool pushOutOfBlocks(Vec3 pos);
	virtual void onCollideWithPlayer(PlayerEntity& entity);
	virtual void applyKnockback(Vec3 direction);
	//TODO: use moveForward, moveStrafe or the new input vector instead of args
	virtual void applyInput(float strafe, float forward, float acceleration);
	virtual void move(Vec3 movement);
	//TODO: Move to LivingEntity
	virtual void dealDamage(int amount);
	virtual void updateFallState(float movedY);
	virtual std::optional<Tag> serializeToNBT();
	virtual void loadFromNBT(Tag& nbt);
};