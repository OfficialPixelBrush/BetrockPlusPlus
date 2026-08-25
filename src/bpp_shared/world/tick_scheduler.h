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

// For scheduling Tick events in the world
struct ScheduledTick {
	int64_t tickDue;
	int64_t sequence; // Insertion order
	Int3 pos;
	BlockType expectedBlock;

	bool operator>(const ScheduledTick& _rhs) const {
		if (tickDue != _rhs.tickDue)
			return tickDue > _rhs.tickDue;
		return sequence > _rhs.sequence;
	}
};

struct PendingEntry {
	TickTime dueTick;
	int64_t sequence;
};

class WorldManager;
struct TickScheduler {
	WorldManager* world = nullptr;
	std::priority_queue<ScheduledTick, std::vector<ScheduledTick>, std::greater<ScheduledTick>> scheduledTicks;
	std::unordered_map<Int3, PendingEntry> pending;

	TickTime currentTick = 0;
	int64_t nextSequence = 0;

	void ScheduleUpdateTick(Int3 _pos, BlockType _block, int _tickDelay) {
		TickTime dueTick = currentTick + TickTime(_tickDelay);
		auto it = pending.find(_pos);
		if (it != pending.end()) {
			return; // an update is already pending for this block
		}
		auto sequence = nextSequence++;
		scheduledTicks.push({ dueTick, sequence, _pos, _block });
		pending[_pos] = { dueTick, sequence };
	}

	void Tick();
};