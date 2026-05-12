/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once

// Base entity struct
struct entity {
	int id = -1; // Entity ID, -1 is an invalid entity
	bool preventEntitySpawning = false;
	entity* riddenByEntity;
	entity* ridingEntity;

	// Position last tick
	double prevPosX;
	double prevPosY;
	double prevPosZ;

	// Position this tick
	double posX;
	double posY;
	double posZ;

	// Velocity
	double motionX;
	double motionY;
	double motionZ;

	// Look direction
	float rotationYaw;
	float rotationPitch;

	// Look direction last tick
	float prevRotationYaw;
	float prevRotationPitch;

	// Collider
	AABB boundingBox{ 0, 0, 0, 0, 0, 0 };

	bool onGround = false;
	bool collidedHorizontally = false;
	bool collidedVertically = false;
	bool collided = false;
	bool beenAttacked = false;
	bool inWeb = false;
	bool hasPhysics = true;
	bool isDead = false;

	// Vertical offset from posY to the bottom of the bounding box (feet position)
	float yOffset = 0.0F;

	// Collision box dimensions in blocks
	float width = 0.6F;
	float height = 1.8F;

	// Distance walked last tick
	float prevDistanceWalked = 0.0f;
	
	// Distance walked this tick
	float distanceWalked = 0.0f;

	float fallDistance = 0.0f;

	// Position the tick before last
	double prevPrevPosX;
	double prevPrevPosY;
	double prevPrevPosZ;
 };