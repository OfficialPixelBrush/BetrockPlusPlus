/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include "world/world.h"
#include "entities/entity_manager.h"

// Entity tracker so we can send entity updates to the right players. This is server side only annoyingly enough. 
// I am not entirely happy with how this is done but notch demands we have several packet types for each type of entity
struct EntityTracker {
	
};