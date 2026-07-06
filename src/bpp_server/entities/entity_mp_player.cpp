/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "entity_mp_player.h"
#include "../player_conn/player_session.h"

// We ignore physics for the player entity and just grab what the client tells us
void EntityMPPlayer::tick() {
	if (!session)
		return;
	Vec3 claimed = session->position.pos;

	posX = claimed.x;
	posY = claimed.y;
	posZ = claimed.z;
}