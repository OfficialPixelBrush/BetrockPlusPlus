/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
 */
#pragma once

#include "helpers/AABB.h"
#include "world/world.h"

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

	virtual void tick() {
		ticksExisted++;
	}
};