/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
 */
#pragma once
#include "helpers/AABB.h"
#include "numeric_structs.h"
#include "packet_data.h"
#include "world/world.h"
#include <vector>


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
const float STEP_HEIGHT = 0.5;
const float DEFAULT_BLOCK_SLIPPERINESS = 0.6f;
const float NORMAL_FRICTION_CUBED = 0.16277136f;
const float AIR_ACCELERATION = 0.02f;
const float INPUT_DECAY = 0.98f;
const float SNEAK_SPEED_MODIFIER = 0.3f;

struct Entity {
	// World pointer
	WorldManager* world = nullptr;

	// Identity
	int id = -1; // -1 = not yet spawned
	bool isDead = false;
	int ticksExisted = 0;

	// Riding
	Entity* ridingEntity = nullptr;
	Entity* riddenByEntity = nullptr;

	// Position
	double posX = 0.0;
	double posY = 0.0;
	double posZ = 0.0;

	// Position at the start of the current tick
	double prevPosX = 0.0;
	double prevPosY = 0.0;
	double prevPosZ = 0.0;

	// Velocity
	double motionX = 0.0;
	double motionY = 0.0;
	double motionZ = 0.0;

	// Look direction
	float rotationYaw = 0.0f;
	float rotationPitch = 0.0f;

	float prevRotationYaw = 0.0f;
	float prevRotationPitch = 0.0f;

	// Collision
	AABB collider;
	Int3 bucketPos = { 0, 0, 0 }; // The bucket this entity is currently in (for spatial partitioning)

	// Width/height of the collision box in blocks.
	float width = 0.6f;
	float height = 1.8f;

	// Vertical offset from posY down to the bottom of the bounding box
	float yOffset = 0.0f;

	// How high a block face this entity can step onto without jumping.
	float stepHeight = 0.5f;

	// Collision state
	bool onGround = false;
	bool collided = false;
	bool collidedHorizontally = false;
	bool collidedVertically = false;

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
	bool isJumping = false;

	float fallDistance = 0.0f;

	int nextStepDistance = 0;

	// Accumulated walk distance this tick (unused rn its mostly for the client)
	float distanceWalkedModified = 0.0f;
	float prevDistanceWalkedModified = 0.0f;
    float ySize = 0.0f;

	float moveForward = 0.0f; // Forward/backward input axis
	float moveStrafe = 0.0f;  // Left/right input axis

    // Accumulated walk distance this tick (unused rn its mostly for the client)
    float distanceWalkedModified = 0.0f;
    float prevDistanceWalkedModified = 0.0f;

	// Fire
	int fire = 0;           // Ticks remaining on fire; 0 = not on fire
	bool inFire = false;    // Currently touching a fire/lava block
	int fireResistance = 1; // Ticks of immunity after catching fire

	// Combat
	bool beenAttacked = false;
	int hurtResistantTime = 0;  // Invincibility frames after being hit
	float attackedAtYaw = 0.0f; // Yaw from which the last attack came

	// Spawning
	bool preventEntitySpawning = false;
	bool isFirstUpdate = true; // True only on the very first tick

	virtual void applyMetadata(const std::vector<PacketData::EntityMetadata::DataEntry>& metadata) {}

    // main per-tick movement update: reads input, applies jumping, dispatches to the appropriate movement path
	virtual void tick() {
        ticksExisted++;

        /*
        // input axes are decayed every tick before new input is added
        moveForward *= INPUT_DECAY;
        moveStrafe *= INPUT_DECAY;

        if (sneaking) {
            moveForward *= SNEAK_SPEED_MODIFIER;
            moveStrafe *= SNEAK_SPEED_MODIFIER;
        }

        if (jumping) {
            if (inWater || inLava) {
                motionY += FLUID_JUMP_BOOST;
            }
            else if (onGround) {
                motionY = JUMP_VELOCITY;
            }
        }

        if (inWater) {
            moveInFluid(WATER_DRAG);
        }
        else if (inLava) {
            moveInFluid(LAVA_DRAG);
        }
        else {
            // friction depends on the block underfoot; slippery blocks reduce both braking and acceleration
            float friction = onGround
                ? belowBlock.slipperiness * HORIZONTAL_FRICTION
                : HORIZONTAL_FRICTION;

            // acceleration is tuned so that on normal ground (slipperiness 0.6) it equals exactly 0.1
            float acceleration = onGround
                ? 0.1 * (NORMAL_FRICTION_CUBED / (friction * friction * friction))
                : AIR_ACCELERATION;

            applyInput(moveStrafe, moveForward, acceleration);

            if (onLadder) {
                motionX = std::clamp(float(motionX), -LADDER_MAX_HORIZONTAL, LADDER_MAX_HORIZONTAL);
                motionY = std::max(float(motionY), sneaking ? LADDER_SNEAK_DESCENT : -LADDER_MAX_DESCENT);
                motionZ = std::clamp(float(motionZ), -LADDER_MAX_HORIZONTAL, LADDER_MAX_HORIZONTAL);
                fallDistance = 0;
            }

            move({motionX, motionY, motionZ});

            // climb upward when pressing into a ladder
            if (collidedHorizontally && onLadder) {
                motionY = LADDER_WALL_BOOST;
            }

            motionY -= GRAVITY;
            motionX *= friction;
            motionY *= VERTICAL_FRICTION;
            motionZ *= friction;
        }
        */
    }

    // applies an impulse pushing the entity away from its attacker
    void applyKnockback(Vec3 direction) {
		motionX *= KNOCKBACK_VELOCITY_DAMPENING; motionY *= KNOCKBACK_VELOCITY_DAMPENING; motionZ *= KNOCKBACK_VELOCITY_DAMPENING;
        motionX -= direction.x * HORIZONTAL_KNOCKBACK;
        motionZ -= direction.z * HORIZONTAL_KNOCKBACK;
        motionY = std::min(float(motionY + VERTICAL_KNOCKBACK), VERTICAL_KNOCKBACK);
    }

    // converts strafe/forward input into a velocity impulse along the entity's facing direction
    void applyInput(float strafe, float forward, float acceleration) {
        float length = sqrt((strafe * strafe) + (forward * forward));

        if (length < 0.01) {
            return;
        }

        if (length < 1.0) {
            length = 1.0;
        }

        strafe /= length;
        forward /= length;

        motionX += (strafe * cos(rotationYaw) - forward * sin(rotationYaw)) * acceleration;
        motionZ += (forward * cos(rotationYaw) + strafe * sin(rotationYaw)) * acceleration;
    }

    // moves the entity by the given vector, handling cobwebs, sneaking, block collisions, and fall state
    void move(Vec3 movement) {
        ySize *= 0.4;

        if (inWeb) {
            inWeb = false;
            movement.x *= COBWEB_HORIZONTAL_DRAG;
            movement.y *= COBWEB_VERTICAL_DRAG;
            movement.z *= COBWEB_HORIZONTAL_DRAG;
			motionX = 0.0f; motionY = 0.0f; motionZ = 0.0f;
        }

        Vec3 original = movement;

        if (onGround && sneaking) {
            sneakClipMovement(movement);
        }

        resolveCollisions(movement, original);
        updateFallState(movement.y);
    }

    void sneakClipMovement(Vec3& movement) {}
	void resolveCollisions(Vec3& movement, const Vec3& original) {}
	void dealDamage(int amount) {}

    // accumulates fall distance while airborne and deals damage on landing
    void updateFallState(float movedY) {
        if (onGround) {
            if (fallDistance > FALL_DAMAGE_FLOOR) {
                dealDamage(ceil(fallDistance - FALL_DAMAGE_FLOOR));
            }

            fallDistance = 0;
        }
        else if (movedY < 0) {
            fallDistance -= movedY;
        }
    }
};