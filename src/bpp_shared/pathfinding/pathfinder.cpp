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
		it->second.m_pos = pos;
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
	bottomPos.m_y -= 1;
	bool isAir = m_world->isAirBlock(pos);
	bool isNormal = m_world->isBlockNormalCube(bottomPos);
	return isAir && isNormal;
}

std::optional<Int3> Pathfinder::getNeighbour(Int3 from, Int2 dir) {
	Int3 pos = from;
	pos.m_x += dir.m_x;
	pos.m_z += dir.m_y;

	// Step up one block if blocked.
	if (!m_world->isAirBlock(pos)) {
		pos.m_y++;

		if (!m_world->isAirBlock(pos))
			return std::nullopt;
	}

	constexpr int MAX_FALL_DISTANCE = 3;
	int fallDistance = 0;
	while (m_world->isAirBlock({ pos.m_x, pos.m_y - 1, pos.m_z })) {
		pos.m_y--;
		fallDistance++;

		if (fallDistance > MAX_FALL_DISTANCE)
			return std::nullopt;
	}

	if (!posValid(pos))
		return std::nullopt;

	return pos;
}

inline int manhattan(const Int3& a, const Int3& b) noexcept {
	return std::abs(a.m_x - b.m_x) + std::abs(a.m_y - b.m_y) + std::abs(a.m_z - b.m_z);
}

const Int2 CARDINAL_DIRS[4] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };

std::vector<Int3> Pathfinder::findPath(Int3 start, Int3 goal) {
	reset();

	if (!posValid(start) || !posValid(goal))
		return {};

	Node* startNode = openNode(start);
	Node* goalNode = openNode(goal);

	startNode->m_g = 0;
	startNode->m_f = manhattan(start, goal);

	open.push({ startNode->m_f, startNode });

	while (!open.empty()) {
		Node* current = open.top().m_node;
		open.pop();

		if (current->m_closed)
			continue;

		current->m_closed = true;

		if (current == goalNode)
			break;

		for (const auto& dir : CARDINAL_DIRS) {
			std::optional<Int3> nextPos = getNeighbour(current->m_pos, dir);
			if (!nextPos)
				continue;

			Node* next = openNode(*nextPos);

			if (next->m_closed)
				continue;

			int g = current->m_g + 1;

			if (g < next->m_g) {
				next->m_g = g;
				next->m_f = g + manhattan(*nextPos, goal);
				next->m_parent = current;

				open.push({ next->m_f, next });
			}
		}
	}

	if (!goalNode->m_closed)
		return {};

	std::vector<Int3> path;

	for (Node* n = goalNode; n != startNode; n = n->m_parent)
		path.push_back(n->m_pos);

	std::reverse(path.begin(), path.end());

	return path;
}