/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "tick_scheduler.h"
#include "world.h"

void TickScheduler::tick() {
	if (!this->m_world)
		return;

	while (!this->m_scheduledTicks.empty() && m_scheduledTicks.top().m_tickDue <= m_currentTick) {
		ScheduledTick entry = m_scheduledTicks.top();
		m_scheduledTicks.pop();

		// Did we reschedule this position?
		auto it = m_pending.find(entry.m_pos);
		if (it != m_pending.end() && it->second == entry.m_sequence)
			m_pending.erase(it);

		// Has the block changed since we scheduled this tick?
		if (m_world->getBlockId(entry.m_pos) == entry.m_expectedBlock) {
			if (auto fn = Blocks::blockBehaviors[entry.m_expectedBlock].m_onTick)
				fn(*m_world, entry.m_pos, 0, m_world->m_rand);
		}
	}
	m_currentTick++;
}