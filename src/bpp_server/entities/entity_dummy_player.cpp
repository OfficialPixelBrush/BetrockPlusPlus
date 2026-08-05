/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "entity_dummy_player.h"
#include "../player_conn/player_session.h"
#include "entities/entity.h"
#include "entities/entity_item.h"
#include "items.h"

namespace {
const std::vector<std::string> kControversialStatements = {
	"<Billy> Don't buy temu buttplugs", 
	"<Billy> Put women in the kitchen", 
	"<Billy> Bedrock edition is ass", 
	"<Billy> Guess where my fingers are", 
	"<Billy> I guess you could say I'm a... Silly billy?",
	"<Billy> Shut up dumbass",
	"<Billy> pee pee",
	"<Billy> aaa eee ooo, ooo eee aaa",
	"<Billy> Feed me",
	"<Billy> I could really go for a foot long glick" };

const std::string& PickLine(const std::vector<std::string>& _lines, Java::Random& _rand) {
	return _lines[size_t(_rand.NextInt(int(_lines.size())))];
}
} // namespace

bool DummyMPPlayer::SeekFood() {
	if (!entityManager || !currentPath.empty())
		return false;

	// Only bother hunting for food if he doesn't already have some
	bool alreadyHasFood = false;
	for (auto& stack : session->inventory.slots) {
		if (stack.id == Items::PORKCHOP || stack.id == Items::PORKCHOP_COOKED) {
			alreadyHasFood = true;
			break;
		}
	}
	if (alreadyHasFood)
		return false;

	AABB searchBox = collider.Expand(10.0, 4.0, 10.0);
	auto nearby = entityManager->GetEntitiesWithinAabbExcluding(searchBox, this->id);

	std::shared_ptr<Entity> closestFood = nullptr;
	double closestDistSq = 0.0;
	for (const auto& entity : nearby) {
		if (entity->type != EntityType::ITEM || entity->isDead)
			continue;
		auto& itemEntity = static_cast<ItemEntity&>(*entity);
		if (itemEntity.itemStack.id != Items::PORKCHOP && itemEntity.itemStack.id != Items::PORKCHOP_COOKED)
			continue;

		double distSq = position.DistanceSquared(entity->position);
		if (!closestFood || distSq < closestDistSq) {
			closestFood = entity;
			closestDistSq = distSq;
		}
	}

	if (!closestFood)
		return false;

	Int3 goal = { MathHelper::FloorDouble(closestFood->position.x), MathHelper::FloorDouble(closestFood->position.y),
		          MathHelper::FloorDouble(closestFood->position.z) };
	SetGoal(goal);
	return true;
}

void DummyMPPlayer::MaybeSayThing() {
	if (chatCooldown > 0) {
		chatCooldown--;
		return;
	}

	if (!entityManager || !entityManager->GetClosestPlayerWithin(position, 16)) {
		chatCooldown += int(rand.NextFloat() * 100);
		return;
	}
	if (rand.NextInt(150) != 0) {
		chatCooldown += int(rand.NextFloat() * 100);
		return;
	}

	std::string line;
	line = PickLine(kControversialStatements, rand);

	server.SendGlobalChatMessage(line);
	chatCooldown = (20 * 120);
}

void DummyMPPlayer::Wonder() {
	if (SeekFood())
		return;

	bool hasPath = !currentPath.empty();

	bool shouldWander = (!hasPath && rand.NextInt(80) == 0) || rand.NextInt(80) == 0;

	// Billy is TERRIFIED of death
	if (GetHeartsHealth() <= 6) {
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

	// He's terrified of death
	if (belowBlockId == BLOCK_FIRE || belowBlockId == BLOCK_LAVA_STILL || belowBlockId == BLOCK_LAVA_FLOWING ||
	    belowBlockId == BLOCK_CACTUS)
		return -1000.0f;

	if (belowBlockId == BLOCK_AIR) {
		return -5.0f;
	}

	// Otherwise score by how lit the spot is
	int light = std::max(world->GetSkyLight(_pos), world->GetBlockLight(_pos));
	return (float(light) / 15.0f) - 0.5f;
}

void DummyMPPlayer::Tick() {
	if (!session) {
		dummySession.connState = ConnectionState::Playing;
		session = &dummySession;
		return;
	}

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
		this->movementSpeed = GetHeartsHealth() <= 6 ? 1.2f : 0.7f;
		Wonder();
		FollowPath();
		MaybeSayThing();
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