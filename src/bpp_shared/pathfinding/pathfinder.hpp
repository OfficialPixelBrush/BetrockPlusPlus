/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/
#pragma once

#include "numeric_structs.h"
#include "world.h"
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <vector>

typedef int(heuristic_fn)(const Int3&, const Int3&);

struct Node {
	Int3 pos;

	int g = INT32_MAX;
	int f = INT32_MAX;

	bool closed = false;

	Node* parent = nullptr;
};

class Pathfinder {
public:
	WorldManager* world = nullptr;

	Pathfinder(){};
	Pathfinder(WorldManager* world) : world(world) {};
	[[nodiscard]] std::vector<Int3> findPath(Int3 start, Int3 goal);

private:

	struct PQNode {
		int f;
		Node* node;

		bool operator>(const PQNode& rhs) const {
			return f > rhs.f;
		}
	};

	std::priority_queue<PQNode, std::vector<PQNode>, std::greater<PQNode>> open;

	std::unordered_map<Int3, Node> nodes;

	Node* openNode(Int3 pos);
	bool posValid(Int3 pos);
	std::optional<Int3> getNeighbour(Int3 from, Int2 dir);
	void reset();
};