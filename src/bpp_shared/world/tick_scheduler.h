/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "base_types.h"
#include "blocks.h"
#include "logger.h"
#include <cstdint>
#include <numeric_structs.h>
#include <queue>
#include <unordered_map>
#include <vector>

// For scheduling tick events in the world
struct ScheduledTick {
	int64_t m_tickDue;
	int64_t m_sequence; // Insertion order
	Int3 m_pos;
	BlockType m_expectedBlock;

	bool operator>(const ScheduledTick& rhs) const {
		if (m_tickDue != rhs.m_tickDue)
			return m_tickDue > rhs.m_tickDue;
		return m_sequence > rhs.m_sequence;
	}
};

struct WorldManager;
struct TickScheduler {
	WorldManager* m_world = nullptr;
	std::priority_queue<ScheduledTick, std::vector<ScheduledTick>, std::greater<ScheduledTick>> m_scheduledTicks;
	std::unordered_map<Int3, TickTime> m_pending;

	TickTime m_currentTick = 0;
	int64_t m_nextSequence = 0;

	void scheduleUpdateTick(Int3 pos, BlockType block, int tickDelay) {
		if (m_pending.contains(pos) && m_pending[pos] == m_currentTick + TickTime(tickDelay)) {
			return;
		}
		auto sequence = m_nextSequence++;
		m_scheduledTicks.push({ m_currentTick + tickDelay, sequence, pos, block });
		m_pending[pos] = sequence;
	}

	void tick();
};