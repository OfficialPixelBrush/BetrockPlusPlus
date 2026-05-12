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
	boolean preventEntitySpawning = false;
	entity* riddenByEntity;
	entity* ridingEntity;
	double prevPosX;
	double prevPosY;
	double prevPosZ;
	double posX;
	double posY;
	double posZ;
	double motionX;
	double motionY;
	double motionZ;
	float rotationYaw;
	float rotationPitch;
	float prevRotationYaw;
	float prevRotationPitch;
};