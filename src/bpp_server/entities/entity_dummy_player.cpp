/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "../player_conn/player_session.h"
#include "entity_dummy_player.h"

void DummyMPPlayer::Wonder() {
	bool hasPath = !currentPath.empty();

	bool shouldWander = (!hasPath && rand.NextInt(80) == 0) || rand.NextInt(80) == 0;

	// Billy is TERRIFIED of death
	if (GetHeartsHealth() <= 3) {
		shouldWander = !hasPath || rand.NextInt(30) == 0;
	}

	if (!shouldWander)
		return;

	Int3 origin = { MathHelper::FloorDouble(position.x), MathHelper::FloorDouble(position.y),
		            MathHelper::FloorDouble(position.z) };

	bool found = false;
	Int3 best{};
	float bestWeight = -99999.0f;

	// Sample 10 random points
	for (int i = 0; i < 10; i++) {
		Int3 candidate = { origin.x + rand.NextInt(13) - 6, origin.y + rand.NextInt(7) - 3,
			               origin.z + rand.NextInt(13) - 6 };

		float weight = GetWanderWeight(candidate);
		if (weight > bestWeight) {
			bestWeight = weight;
			best = candidate;
			found = true;
		}
	}

	if (found)
		SetGoal(best);
}

float DummyMPPlayer::GetWanderWeight(Int3 _pos) {
	// Grass underfoot always wins
	Int3 below = _pos;
	below.y -= 1;
	BlockType belowBlockId = world->GetBlockId(below);
	if (belowBlockId == BLOCK_GRASS || belowBlockId == BLOCK_PLANKS || belowBlockId == BLOCK_COBBLESTONE ||
	    belowBlockId == BLOCK_STONE)
		return 10.0f;

	if (belowBlockId == BLOCK_FIRE || belowBlockId == BLOCK_AIR || belowBlockId == BLOCK_LAVA_STILL) {
		// He is a little suicidal
		return int(rand.NextFloat() * 10.0f);
	}

	// Otherwise score by how lit the spot is
	int light = std::max(world->GetSkyLight(_pos), world->GetBlockLight(_pos));
	return (float(light) / 15.0f) - 0.5f;
}

void DummyMPPlayer::Tick() {
	if (!session) {
		dummySession.connState = ConnectionState::Playing;
		dummySession.username = "Billy";
		session = &dummySession;
		return;
	}

	this->movementSpeed = GetHeartsHealth() <= 3 ? 32 : 7;

	// Set our held item and armor
	// Slots 5 -> 8 are for armor
	ItemStack none = {};
	auto heldItemPtr = session->inventory.GetHeldItem();
	this->heldItem = session->inventory.GetHeldItem() ? *heldItemPtr : none;
	for (int i = 0; i < 4; i++) {
		auto armorSlotPtr = session->inventory.GetStackInSlot(5 + i);
		this->armor[i] = armorSlotPtr ? *armorSlotPtr : none;
	}

	// Do living entity stuff
	MobileEntity::Tick();

	// Billy should try and eat food and stuff
	if ((age - 14) % 15 == 0) {
		for (auto& stack : session->inventory.slots) {
			// Eat food
			if (stack.id == Items::PORKCHOP || stack.id == Items::PORKCHOP_COOKED) {
				Items::EatFood(*session, &stack, *this);
			}
		}
	}

	if (EntityAlive()) {
		Wonder();
		FollowPath();
	}

	// If we fell out of the world then die
	if (position.y < -64.0)
		OnDeath();

	// Tell entities we collided with a player
	if (entityManager) {
		auto entitiesCollidingWith = entityManager->GetEntitiesWithinAabbExcluding(collider.Expand(1.0, 0.0, 1.0),
		                                                                           this->id);
		for (const auto& entity : entitiesCollidingWith) {
			if (!entity->isDead)
				entity->OnCollideWithPlayer(*this);
		}
	}
}