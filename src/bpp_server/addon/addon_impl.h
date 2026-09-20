/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

// This is the internal C++ implementation of the API.

#pragma once

#include "addon_api.h"
#include <unordered_map>

class PlayerSession;
class WorldManager;

struct Addon;

struct bp_player {
	PlayerSession* session;
	std::unordered_map<Addon*, void*> addonData;
};

struct bp_world {
	WorldManager* manager;
};

bp_api MakeAddonAPI();