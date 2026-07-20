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

Node* Pathfinder::openNode(Int3 _pos) {
	auto [it, inserted] = nodes.try_emplace(_pos);

	if (inserted) {
		it->second.pos = _pos;
	}

	return &it->second;
}

void Pathfinder::reset() {
	while (!open.empty())
		open.pop();

	nodes.clear();
}

bool Pathfinder::posValid(Int3 _pos) {
	Int3 bottomPos = _pos;
	bottomPos.y -= 1;
	bool isAir = world->isAirBlock(_pos);
	bool isNormal = world->isBlockNormalCube(bottomPos);
	return isAir && isNormal;
}

std::optional<Int3> Pathfinder::getNeighbour(Int3 _from, Int2 _dir) {
	Int3 pos = _from;
	pos.x += _dir.x;
	pos.z += _dir.y;

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

inline int Manhattan(const Int3& _a, const Int3& _b) noexcept {
	return std::abs(_a.x - _b.x) + std::abs(_a.y - _b.y) + std::abs(_a.z - _b.z);
}

const Int2 CARDINAL_DIRS[4] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };

std::vector<Int3> Pathfinder::findPath(Int3 _start, Int3 _goal) {
	reset();

	if (!posValid(_start) || !posValid(_goal))
		return {};

	Node* startNode = openNode(_start);
	Node* goalNode = openNode(_goal);

	startNode->g = 0;
	startNode->f = Manhattan(_start, _goal);

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
				next->f = g + Manhattan(*nextPos, _goal);
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