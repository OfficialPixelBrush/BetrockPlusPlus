/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "blocks.h"
#include "logger.h"
#include <numeric_structs.h>
#include <cstdint>
#include <queue>  
#include <vector> 
#include <unordered_map>

// For scheduling tick events in the world
struct ScheduledTick {
	int64_t tickDue;
	int64_t sequence; // Insertion order
	Int3 pos;
	BlockType expectedBlock;

	bool operator>(const ScheduledTick& rhs) const {
		if (tickDue != rhs.tickDue)
			return tickDue > rhs.tickDue;
		return sequence > rhs.sequence;
	}
};

struct WorldManager;
struct TickScheduler {
	WorldManager* world = nullptr;
	std::priority_queue<ScheduledTick, std::vector<ScheduledTick>, std::greater<ScheduledTick>> scheduledTicks;
	std::unordered_map<Int3, uint64_t> pending;

	int64_t currentTick = 0;
	int64_t nextSequence = 0;

	void scheduleUpdateTick(Int3 pos, BlockType block, int tickDelay) {
		if (pending.contains(pos) && pending[pos] == currentTick + tickDelay) {
			GlobalLogger().info << "Blocked duplicate schedule at " << pos << "\n";
			return;
		}
		auto sequence = nextSequence++;
		GlobalLogger().info << "Scheduling " << pos << " seq=" << sequence << " due=" << (currentTick + tickDelay)
		                    << "\n";
		scheduledTicks.push({ currentTick + tickDelay, sequence, pos, block });
		pending[pos] = sequence;
	}

	void tick();
};