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
	Pathfinder(WorldManager* _world) : world(_world) {};
	[[nodiscard]] std::vector<Int3> FindPath(Int3 _start, Int3 _goal);

private:

	struct PQNode {
		int f;
		Node* node;

		bool operator>(const PQNode& _rhs) const {
			return f > _rhs.f;
		}
	};

	std::priority_queue<PQNode, std::vector<PQNode>, std::greater<PQNode>> open;

	std::unordered_map<Int3, Node> nodes;

	Node* OpenNode(Int3 _pos);
	bool PosValid(Int3 _pos);
	std::optional<Int3> GetNeighbour(Int3 _from, Int2 _dir);
	void Reset();
};