/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/
#include <algorithm>
#include <optional>
#include <queue>
#include <vector>

#include "numeric_structs.h"
#include "pathfinder.hpp"

Node* Pathfinder::openNode(Int3 pos) {
	auto [it, inserted] = nodes.try_emplace(pos);

	if (inserted) {
		it->second.pos = pos;
	}

	return &it->second;
}

void Pathfinder::reset() {
	while (!open.empty())
		open.pop();

	nodes.clear();
}

bool Pathfinder::posValid(Int3 pos) {
	Int3 bottomPos = pos;
	bottomPos.y -= 1;
	bool isAir = world->isAirBlock(pos);
	bool isNormal = world->isBlockNormalCube(bottomPos);
	return isAir && isNormal;
}

std::optional<Int3> Pathfinder::getNeighbour(Int3 from, Int2 dir) {
	Int3 pos = from;
	pos.x += dir.x;
	pos.z += dir.y;

	// Step up one block if blocked.
	if (!world->isAirBlock(pos)) {
		pos.y++;

		if (!world->isAirBlock(pos))
			return std::nullopt;
	}

	constexpr int MAX_FALL_DISTANCE = 3;
	int fallDistance = 0;
	while (world->isAirBlock({ pos.x, pos.y - 1, pos.z })) {
		pos.y--;
		fallDistance++;

		if (fallDistance > MAX_FALL_DISTANCE)
			return std::nullopt;
	}

	if (!posValid(pos))
		return std::nullopt;

	return pos;
}

inline int manhattan(const Int3& a, const Int3& b) noexcept {
	return std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z);
}

const Int2 CARDINAL_DIRS[4] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };

std::vector<Int3> Pathfinder::findPath(Int3 start, Int3 goal) {
	reset();

	if (!posValid(start) || !posValid(goal))
		return {};

	Node* startNode = openNode(start);
	Node* goalNode = openNode(goal);

	startNode->g = 0;
	startNode->f = manhattan(start, goal);

	open.push({ startNode->f, startNode });

	while (!open.empty()) {
		Node* current = open.top().node;
		open.pop();

		if (current->closed)
			continue;

		current->closed = true;

		if (current == goalNode)
			break;

		for (const auto& dir : CARDINAL_DIRS) {
			std::optional<Int3> nextPos = getNeighbour(current->pos, dir);
			if (!nextPos)
				continue;

			Node* next = openNode(*nextPos);

			if (next->closed)
				continue;

			int g = current->g + 1;

			if (g < next->g) {
				next->g = g;
				next->f = g + manhattan(*nextPos, goal);
				next->parent = current;

				open.push({ next->f, next });
			}
		}
	}

	if (!goalNode->closed)
		return {};

	std::vector<Int3> path;

	for (Node* n = goalNode; n != startNode; n = n->parent)
		path.push_back(n->pos);

	std::reverse(path.begin(), path.end());

	return path;
}