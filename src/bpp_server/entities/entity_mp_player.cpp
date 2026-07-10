/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "entity_mp_player.h"
#include "../player_conn/player_session.h"
#include "networking/network_stream.h"
#include "networking/packets.h"

bool EntityMPPlayer::pickupItem(ItemStack& stack, EntityId entityId) {
	if (this->session->inventory.pickupItem(stack)) {
		Packet::CollectItem pkt;
		pkt.collector_entity_id = this->id;
		pkt.item_entity_id = entityId;
		this->session->entityTracker->sendPacketToViewers(pkt, this->id);
		pkt.Serialize(this->session->stream);
		return true;
	}
}

// We ignore physics for the player entity and just grab what the client tells us
void EntityMPPlayer::tick() {
	if (!session)
		return;
	Vec3 claimed = session->position.pos;

	posX = claimed.x;
	posY = claimed.y;
	posZ = claimed.z;
	rotationYaw = session->rotation.x;
	rotationPitch = session->rotation.y;

	rebuildCollider();

	// Tell entities we collided with them
	if (entityManager) {
		AABB colliderCopy = collider.expand(1.0, 0.0, 1.0);
		auto entitiesCollidingWith = entityManager->getEntitiesWithinAABBExcluding(colliderCopy, this->id);
		for (auto& entity : entitiesCollidingWith) {
			if (entity->collider.intersects(colliderCopy) && !entity->isDead)
				entity->onCollideWithPlayer(*this);
		}
	}
}