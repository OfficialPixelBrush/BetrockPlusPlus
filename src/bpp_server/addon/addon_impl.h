/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

// This is the internal C++ implementation of the API.

#pragma once

#include "addon_api.h"
#include "../player_conn/player_session.h"

struct bp_player {
	PlayerSession &session;
};

bp_api MakeAddonAPI();