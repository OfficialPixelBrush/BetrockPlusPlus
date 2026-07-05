/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "entity_mp_player.h"
#include "../player_conn/player_session.h"

namespace {
// Beta's own client always reports the true feet position (boundingBox.minY)
// on the wire for ongoing movement packets - never the eye-level posY. Real
// Beta never rebuilds its boundingBox from posY during normal play either;
// moveEntity() just offsets whatever box it already has, and posY becomes a
// write-only bookkeeping value. So when validating a network-reported
// position here, we build the collider directly from feet, bypassing
// Entity::rebuildCollider()'s yOffset subtraction - that formula is only
// correct for the one-time spawn/teleport conversion from a ground height,
// not for interpreting values that are already feet.
AABB colliderFromFeet(Vec3 feetPos, float width, float height) {
	double halfWidth = double(width) / 2.0;
	return { feetPos.x - halfWidth, feetPos.y, feetPos.z - halfWidth, feetPos.x + halfWidth, feetPos.y + double(height),
		     feetPos.z + halfWidth };
}
} // namespace

// We ignore physics for the player entity and just grab what the client tells us
void EntityMPPlayer::tick() {
	if (!session)
		return;

	constexpr double INSET_EPSILON = 0.0625;

	Vec3 claimed = session->position.pos;
	Vec3 lastGood = session->lastGoodPos;

	if (!session->hasMoved) {
		double dy = claimed.y - lastGood.y;
		if (claimed.x == lastGood.x && dy * dy < 0.01 && claimed.z == lastGood.z) {
			session->hasMoved = true;
		}
	}

	if (!session->hasMoved) {
		// Client hasn't caught up to where we put it yet
		return;
	}

	// Were we already clear of collisions before this move?
	posX = lastGood.x;
	posY = lastGood.y;
	posZ = lastGood.z;
	collider = colliderFromFeet(lastGood, width, height);
	bool wasClear =
	    world->getCollidingBoundingBoxes(collider.expand(-INSET_EPSILON, -INSET_EPSILON, -INSET_EPSILON)).empty();

	// Run the claimed delta through real collision, starting from our last trusted position.
	Vec3 delta = { claimed.x - lastGood.x, claimed.y - lastGood.y, claimed.z - lastGood.z };
	move(delta);

	// Compare where collision says we should be against what the client claimed.
	double dx = claimed.x - posX;
	double dz = claimed.z - posZ;
	bool wrongMove = (dx * dx + dz * dz) > INSET_EPSILON;

	// Trust the client's claimed position
	posX = claimed.x;
	posY = claimed.y;
	posZ = claimed.z;
	collider = colliderFromFeet(claimed, width, height);
	bool collidesAtClaim =
	    !world->getCollidingBoundingBoxes(collider.expand(-INSET_EPSILON, -INSET_EPSILON, -INSET_EPSILON)).empty();

	if (wasClear && (wrongMove || collidesAtClaim)) {
		// Reject; snap back to the last trusted position.
		GlobalLogger().info << "Rubberbanded!\n";
		posX = lastGood.x;
		posY = lastGood.y;
		posZ = lastGood.z;
		collider = colliderFromFeet(lastGood, width, height);
		session->position.pos = lastGood;

		Packet::PlayerPositionAndRotation correction;
		correction.position = session->position.pos;
		correction.camera_y = session->position.pos.y + PLAYER_EYE_HEIGHT;
		correction.rotation = session->rotation;
		correction.onGround = onGround;
		correction.Serialize(session->stream);
	} else {
		// Accept; this becomes our new trusted position.
		session->lastGoodPos = claimed;
	}
}