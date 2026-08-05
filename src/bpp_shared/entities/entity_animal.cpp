/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity_animal.h"

void AnimalEntity::OnDeath() {
	// no op, drop items
	return;
}

void AnimalEntity::Wander() {
	bool hasPath = !currentPath.empty();

	bool shouldWander = (!hasPath && rand.NextInt(80) == 0) || rand.NextInt(80) == 0;
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

float AnimalEntity::GetWanderWeight(Int3 _pos) {
	// Grass underfoot always wins
	Int3 below = _pos;
	below.y -= 1;
	if (world->GetBlockId(below) == BlockType::BLOCK_GRASS)
		return 10.0f;

	// Otherwise score by how lit the spot is
	int light = std::max(world->GetSkyLight(_pos), world->GetBlockLight(_pos));
	return (float(light) / 15.0f) - 0.5f;
}