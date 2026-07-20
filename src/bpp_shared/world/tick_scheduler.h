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

struct WorldManager;
struct TickScheduler {
	WorldManager* world = nullptr;
	std::priority_queue<ScheduledTick, std::vector<ScheduledTick>, std::greater<ScheduledTick>> scheduledTicks;
	std::unordered_map<Int3, TickTime> pending;

	TickTime currentTick = 0;
	int64_t nextSequence = 0;

	void ScheduleUpdateTick(Int3 _pos, BlockType _block, int _tickDelay) {
		if (pending.contains(_pos) && pending[_pos] == currentTick + TickTime(_tickDelay)) {
			return;
		}
		auto sequence = nextSequence++;
		scheduledTicks.push({ currentTick + _tickDelay, sequence, _pos, _block });
		pending[_pos] = sequence;
	}

	void Tick();
};