/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "tick_scheduler.h"
#include "world.h"

void TickScheduler::tick() {
	if (!this->world)
		return;

	while (!this->scheduledTicks.empty() && scheduledTicks.top().tickDue <= currentTick) {
		ScheduledTick entry = scheduledTicks.top();
		scheduledTicks.pop();

		// Did we reschedule this position?
		auto it = pending.find(entry.pos);
		if (it != pending.end() && it->second == entry.sequence)
			pending.erase(it);

		// Has the block changed since we scheduled this tick?
		if (world->getBlockId(entry.pos) == entry.expectedBlock) {
			if (auto fn = Blocks::blockBehaviors[entry.expectedBlock].onTick)
				fn(*world, entry.pos, 0, world->rand);
		}
	}
	currentTick++;
}